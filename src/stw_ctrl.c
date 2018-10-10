#include <math.h>
#include <stdbool.h>
#include "stow_controller.h"

static void *ctrl_alg(void *tid);
static int *ctrl_stat(char *addr);
static int update_ctrl_stat(int st, int fault, int ctrl_s, int idx);
static int stat_encode(int *stat);
static void *ctrl_alg(void *tid);
static void *stat_mon(void *arg);
static bool isval(int val, int *buf, int len);

void *stw_ctrl(void *arg){

	int i, j, ret, ret2, st, ost, *ctr_stat, st_del, ust_del, chksum_cfg, chksum_stat, fault, *thrd_num;
	unsigned int *rs, *start;
	float *spd_buf;
	double run_avg_spd;
	char **buf, **cfg_key, *test;
	//modbus_t *mb;
	uint16_t tab_reg16[32];
	pthread_t thr[tctr_cfg.ctr_num+1];

	i = 0;
	ret = 0;
	st_del = ust_del = 0;
	run_avg_spd = 0;
	fault = 0;

	for(;;){

		thrd_num = malloc(tctr_cfg.ctr_num*sizeof(int));

		pthread_mutex_lock(&lock_ctr);
		tctr_cfg.ctrl_rst = 0;
		tctr_cfg.ctr_stat = malloc(tctr_cfg.ctr_num*sizeof(int));

		for(i=0; i<tctr_cfg.ctr_num; i++)
			*(tctr_cfg.ctr_stat+i) = 0;
		pthread_mutex_unlock(&lock_ctr);

		pthread_mutex_lock(&lock_stat);
		stw_stat.tw_stow = malloc(tctr_cfg.ctr_num*sizeof(int));
		pthread_mutex_unlock(&lock_stat);


		for(i=0; i<tctr_cfg.ctr_num; i++){
			*(thrd_num+i) = i;
			ret = pthread_create(&thr[i], NULL, &ctrl_alg, (void *) (thrd_num+i));
			if(ret){
				//handle error
				;
				printf("Failed to create control thread %d for Controller\n", *(thrd_num+i));
			}
		}

		ret = pthread_create(&thr[i+1], NULL, &stat_mon, NULL);
		if(ret){
			//handle error
			;
			printf("Failed to create control thread %d for Controller\n", *(thrd_num+i));
			exit(0);
		}

		for(i=0; i<tctr_cfg.ctr_num; i++){
			pthread_join(thr[i], NULL);
			printf("%d\n", i);
		}

		pthread_join(thr[i+1], NULL);

		free(thrd_num);
	}

	pthread_mutex_lock(&lock_ctr);
	free(tctr_cfg.ctr_stat);
	pthread_mutex_unlock(&lock_ctr);

	pthread_mutex_lock(&lock_stw);
	free(stw_stat.tw_stow);
	pthread_mutex_unlock(&lock_stw);

	//modbus_close(mb);
	//modbus_free(mb);
}


static int *ctrl_stat(char *addr){

	int i, ret, *stat;
	char **buf;
	modbus_t *mb;
	uint16_t tab_reg16[32];

	mb = modbus_new_tcp(addr, tctr_cfg.port);

	modbus_set_slave(mb, tctr_cfg.slv_adr);

	if(modbus_connect(mb) == -1){
		//printf("\nController read connection failed: %s\n\n", modbus_strerror(errno));
		modbus_free(mb);
		stat = malloc(STAT_LEN*sizeof(int));
		*stat = -1;
		return(stat);

	}

	//printf("Reading Controller register(s)\n");
	ret = modbus_read_registers(mb, tctr_cfg.stat_reg_adr, tctr_cfg.reg_num, tab_reg16);

	if(ret == -1){
		//printf("\nFailed to read Controller registers: %s\n\n", modbus_strerror(errno));
		modbus_close(mb);
		modbus_free(mb);
		stat = malloc(STAT_LEN*sizeof(int));
		*stat = -1;
		return(stat);
	}
	else{

		modbus_close(mb);
		modbus_free(mb);

		stat = dtob(tab_reg16[0], STAT_LEN);

		return(stat);
	}
}

static int update_ctrl_stat(int st, int fault, int ctrl_s, int idx){

	//update stow controller and Controller stow status

	int ret;

	ret = pthread_mutex_trylock(&lock_stat);

	if(!ret){
		stw_stat.w_stow = st;

		*(tctr_cfg.ctr_stat+idx) = ctrl_s;

		pthread_mutex_unlock(&lock_stat);
	}

	return(ret);
}


static int stat_encode(int *stat){

	int i, dec;

	dec = 0;

	for(i=0; i<STAT_LEN; i++)
		dec = dec+(int)(*(stat+i)*pow(2,i));

	return(dec);
}

static void *stat_mon(void *arg){

	//monitor stow command and status of motor


	for(;;){

		if(isval(-1, tctr_cfg.ctr_stat, tctr_cfg.ctr_num))
			set_ctrl_fault("CTRL", 0, 1);
		else if(!isval(-1, tctr_cfg.ctr_stat, tctr_cfg.ctr_num)){
			set_ctrl_fault("CTRL", 0,0);
		}

		sleep(1);
		
	}


}

static bool isval(int val, int *buf, int len){

	int i;

	for(i=0; i<len; i++)
		if(*(buf+i) == val)
			return(true);

		return(false);

}

static void *ctrl_alg(void *tid){

/*********

monitor sensor fault status and send to stow if status is not 0

********/

	int st, i, j, ret, ret2, *ctr_stat, fault, idx, flag;
	char *tcp_addr;

	idx = *((int *) tid);

	pthread_mutex_lock(&lock_ctr);

	tcp_addr = *(tctr_cfg.tcp_adr+idx);

	pthread_mutex_unlock(&lock_ctr);

	j = st = fault = 0;
 
	for(;;){

		ctr_stat = ctrl_stat(tcp_addr);


		if(*ctr_stat == -1){

			//set fault code for down Controller
			if(stw_stat.ctrl_fault==0){
				fault = 2;
				//set_ctrl_stat("CTRL", 0, 1);
				flag = 1;
			}
		}
		else{

			//clear fault code for down Controller
			//set_ctrl_stat("CTRL", 0, 0);
			if(stw_stat.ctrl_fault!=0 && fault && flag){
				fault = 0;
				flag = 0;
			}
			else if(stw_stat.ctrl_fault!=0 && !fault && !flag){
				flag = 0;
				fault = stw_stat.ctrl_fault;
			}
			else if(stw_stat.ctrl_fault!=0 && fault && flag){
				flag = 0;
				fault = 0;
			}
			else if(stw_stat.ctrl_fault!=0 && !fault && flag)
				flag = 0;

			/*********************** control algorithm*************************************/
			if(stw_cmd && (!st || !*(ctr_stat+2))){
				


				if(!*(ctr_stat+2)){
					ret2 = set_ctr(tcp_addr, 255);
					ctr_stat = ctrl_stat(tcp_addr);
					st = 1;

					if(ret2 > 0){
						st = 1;
						//set_ctrl_stat("CTRL", 1, 0);
					}
					else if(ret2 <0){
						//handle stow error
						//set_ctrl_stat("CTRL", 1, 1);
					}
				}
				else if(*(ctr_stat+2) && !st)
					st = 1;
				else if(*(ctr_stat+2) && st)
					//exit to loop
					;
				else if(ctr_stat == NULL)
					//handle poll error
					;
			}
			else if(!stw_cmd && (st || *(ctr_stat+2))){


				if(*(ctr_stat+2)){
					ret2 = set_ctr(tcp_addr, 0);
					ctr_stat = ctrl_stat(tcp_addr);
					st = 0;

					if(ret2 > 0)
						st = 0;
					else if(ret2 <0)
						//handle stow error
						;
				}
				else if(!*(ctr_stat+2) && st)
					st = 0;
				else if(!*(ctr_stat+2) && st)
					//exit to loop
					;
				else if(ctr_stat == NULL)
					//handle poll error
					;
			}
			/*********************end control algorithm****************************/
		}

		j++;

		if(j==1 && *ctr_stat!=-1){
			j = 0;
			update_ctrl_stat(stw_cmd, fault, stat_encode(ctr_stat), idx);
			free(ctr_stat);
		}
		else if (j==1 && *ctr_stat==-1){
			j = 0;
			update_ctrl_stat(stw_cmd, fault, *ctr_stat, idx);
		}
			sleep(5);
		//free(ctr_stat);

		if(tctr_cfg.ctrl_rst)
			return(0);
	}
}
