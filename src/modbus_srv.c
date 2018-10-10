#include <time.h>
#include "stow_controller.h"

/*
Address 0xc8 + 0x7
-Wind stow command (1/0)
-Snow stow command (1/0)
-Controller wind stow status (1/0)
-Controller snow stow status (1/0)
-Ctrl Fault Code (0,1,2,3,4)
-Snsr Fault Code (0,1,2,3,4)
-Wind speed (X10 m/s)
-Motor Current (X100 A)

Address 0x12c + 0x7
-Stow Overide (1/0)
-Wind speed limit (X100 m/s)
-Stow factor (X100)
-Unstow factor (X100)
-Current limit (X100 A)
-Stow Delay (min)
-Unstow Dealy (min)
*/

const uint16_t STAT_REG_ADDR = 0xc8; //200
const uint16_t STAT_REG_NUM = 0x8; //7
const uint16_t CTRL_REG_ADDR = 0xd0; //208 //0xcF;
const uint16_t CTRL_REG_NUM = 0x9; //9

static int stat_update(modbus_mapping_t *mb_map);

//int modbus_srv(stw_cfg *wstw_cfg){
void *modbus_srv(void *arg){

	int i, j, srv_sckt, rc, qr_reg;
	uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
	modbus_t *ctx;
	modbus_mapping_t *mb_map;

	mb_map = modbus_mapping_new(0, 0, STAT_REG_ADDR+STAT_REG_NUM+CTRL_REG_NUM-1, 0);

	ctx = modbus_new_tcp("127.0.0.1", 502);

	rc = 0;
	j = 0;

	if(mb_map == NULL){
		;
		//handle error
		modbus_free(ctx);
	//	return -1;
	}

	//create and listen to socket
	srv_sckt = modbus_tcp_listen(ctx, 1);

	mb_map->tab_registers[CTRL_REG_ADDR-1] = 0;
	mb_map->tab_registers[CTRL_REG_ADDR] = wstw_cfg.spd_lim*100;
	mb_map->tab_registers[CTRL_REG_ADDR+1] = wstw_cfg.stow_fac*100;
	mb_map->tab_registers[CTRL_REG_ADDR+2] = wstw_cfg.unstow_fac*100;
	mb_map->tab_registers[CTRL_REG_ADDR+3] = wstw_cfg.crnt_lim*100;
	mb_map->tab_registers[CTRL_REG_ADDR+4] = wstw_cfg.stow_del*100;
	mb_map->tab_registers[CTRL_REG_ADDR+5] = wstw_cfg.unstow_del*100;
	mb_map->tab_registers[CTRL_REG_ADDR+6] = wstw_cfg.sstow_fac*100;
	mb_map->tab_registers[CTRL_REG_ADDR+7] = wstw_cfg.sustow_fac*100;

	pthread_mutex_lock(&lock_stw);

	wstw_cfg.chksum = 0;

	for(i=CTRL_REG_ADDR-1; i<CTRL_REG_ADDR+STAT_REG_NUM-1; i++)
		wstw_cfg.chksum+=mb_map->tab_registers[i];

	pthread_mutex_unlock(&lock_stw);

	j = 0;

	stat_update(mb_map);

	for(;;){

		//accept new connection on modbus socket
		modbus_tcp_accept(ctx, &srv_sckt);
		rc = 0;

		while(rc>=0){

			srand(time(NULL));

			//for(i=STAT_REG_ADDR-1; i<STAT_REG_ADDR+STAT_REG_NUM-1; i++)
				//mb_map->tab_registers[i] = (unsigned int)rand();
				//mb_map->tab_registers[i] = 68;

				j++;
			//	printf("%d\n", j);
				if(j==2){
					j = 0;
					stat_update(mb_map);
				}

			rc = modbus_receive(ctx, query);
			
			qr_reg = comb_byte(*(query+8), *(query+9));

			if(qr_reg >= STAT_REG_ADDR-1 && qr_reg < STAT_REG_ADDR+STAT_REG_NUM-1 && *(query+7) == 16){
				modbus_reply_exception(ctx, query, 1);
			}
			else if(qr_reg < STAT_REG_ADDR-1 || qr_reg > CTRL_REG_ADDR+CTRL_REG_NUM-2){
				modbus_reply_exception(ctx, query, 2);
			}
			else if(rc>0 && *(query+7) == 16){
				//clear();
				//printf("\n");
				//for(i=0; i<20; i++)
				//	printf(" %d", *(query+i));
				//printf("\n");
				//exit(1);

				pthread_mutex_lock(&lock_stw);

				
				if(((*(query+14)==1 || *(query+14)==0) && *(query+9)==206) || *(query+9)!=206){
					modbus_reply(ctx, query, rc, mb_map);
					wstw_cfg.stow_ovr = mb_map->tab_registers[CTRL_REG_ADDR-1]; //207
					wstw_cfg.spd_lim = (float)mb_map->tab_registers[CTRL_REG_ADDR]/100; //208
					wstw_cfg.stow_fac = (float)mb_map->tab_registers[CTRL_REG_ADDR+1]/100; //209
					wstw_cfg.unstow_fac = (float)mb_map->tab_registers[CTRL_REG_ADDR+2]/100; //210
					wstw_cfg.crnt_lim = (float)mb_map->tab_registers[CTRL_REG_ADDR+3]/100; //211
					wstw_cfg.stow_del = (int)mb_map->tab_registers[CTRL_REG_ADDR+4]/100; //212
					wstw_cfg.unstow_del = (int)mb_map->tab_registers[CTRL_REG_ADDR+5]/100; //213
					wstw_cfg.sstow_fac = (float)mb_map->tab_registers[CTRL_REG_ADDR+6]/100; //214
					wstw_cfg.sustow_fac = (float)mb_map->tab_registers[CTRL_REG_ADDR+7]/100; //215
					wstw_cfg.chksum = 0;

					for(i=CTRL_REG_ADDR-1; i<CTRL_REG_ADDR+STAT_REG_NUM-1; i++)
						wstw_cfg.chksum+=mb_map->tab_registers[i];
				}
				else
					modbus_reply_exception(ctx, query, 3);

				pthread_mutex_unlock(&lock_stw);

			}
			else{
				modbus_reply(ctx, query, rc, mb_map);
			}
	//		else if(rc == -1){
	//			clear();
	//			printf("\n ** Connection Terminated: %s\n", modbus_strerror(errno));
	//		}
		}
	}

//			modbus_mapping_free(mb_map);
//			modbus_close(ctx);
//			modbus_free(ctx);
}

static int stat_update(modbus_mapping_t *mb_map){

	int ret;

	ret = pthread_mutex_trylock(&lock_stw);
//	pthread_mutex_lock(&lock_stw);

	if(!ret){
		mb_map->tab_registers[CTRL_REG_ADDR-1] = wstw_cfg.stow_ovr;
		mb_map->tab_registers[CTRL_REG_ADDR] = wstw_cfg.spd_lim*100;
		mb_map->tab_registers[CTRL_REG_ADDR+1] = wstw_cfg.stow_fac*100;
		mb_map->tab_registers[CTRL_REG_ADDR+2] = wstw_cfg.unstow_fac*100;
		mb_map->tab_registers[CTRL_REG_ADDR+3] = wstw_cfg.crnt_lim*100;
		mb_map->tab_registers[CTRL_REG_ADDR+4] = wstw_cfg.stow_del*100;
		mb_map->tab_registers[CTRL_REG_ADDR+5] = wstw_cfg.unstow_del*100;
		mb_map->tab_registers[CTRL_REG_ADDR+6] = wstw_cfg.sstow_fac*100;
		mb_map->tab_registers[CTRL_REG_ADDR+7] = wstw_cfg.sustow_fac*100;

		pthread_mutex_unlock(&lock_stw);
	}

	ret = pthread_mutex_trylock(&lock_stat);
//	pthread_mutex_lock(&lock_stw);

	if(!ret){
		mb_map->tab_registers[STAT_REG_ADDR-1] = stw_stat.w_stow;
		mb_map->tab_registers[STAT_REG_ADDR] = stw_stat.s_stow;
		mb_map->tab_registers[STAT_REG_ADDR+1] = *stw_stat.tw_stow; //FIX!!! have overall status as this variable is Controller specific
		mb_map->tab_registers[STAT_REG_ADDR+2] = 0; //*stw_stat.ts_stow;
		mb_map->tab_registers[STAT_REG_ADDR+3] = stw_stat.ctrl_fault;
		mb_map->tab_registers[STAT_REG_ADDR+4] = stw_stat.snsr_fault;  //new
		mb_map->tab_registers[STAT_REG_ADDR+5] = stw_stat.spd*100;
		mb_map->tab_registers[STAT_REG_ADDR+6] = stw_stat.crnt*100;

		pthread_mutex_unlock(&lock_stat);
	}

	return(ret);
}

int comb_byte(int minor, int major){

	int i, ret, *comb_bin, *min_bin, *maj_bin;

	comb_bin = malloc(16*sizeof(int));

	maj_bin = dtob(major, 8);
	min_bin = dtob(minor, 8);

	for(i=0; i<8; i++){
		comb_bin[i] = maj_bin[i];
		comb_bin[i+8] = min_bin[i];
	}


	ret = btod(comb_bin, 16);

	free(maj_bin);
	free(min_bin);
	free(comb_bin);

	return(ret);
}
