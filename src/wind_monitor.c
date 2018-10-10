#include <math.h>
#include "stow_controller.h"

#define SENS_BLOCK 32760


static int update_stw_stat(float *spd, double avg_spd, int st);
static void shift_buf(float *spd_buf, int spd_buf_len);
static float avg(float *spd_buf, int avg_len);
static int sensor_stat(modbus_t *mb);

void *wind_monitor(void *arg){

	int i, ret, st, ost, *ctr_stat, st_del, ust_del, chksum_cfg, chksum_stat, fault;
	unsigned int *rs, *start;
	float *spd_buf;
	double run_avg_spd;
	char **buf, **cfg_key, *test;
	modbus_t *mb;
	uint16_t *tab_reg16;
	uint8_t sens_stat;
	FILE *fp;

	i = 0;
	ret = 0;
	st_del = ust_del = 0;
	run_avg_spd = 0;
	fault = 0;
	stw_cmd = 0;
	mb = NULL;


	tab_reg16 = malloc(sizeof(uint16_t));

	//initialize register variable
	*tab_reg16 = 0;

	//open modbus connection to sensor

	while(!mb){
		mb = modbus_new_rtu("/dev/ttts3", 19200, 'N', 8, 1);
		//set anemometer status code (SBC hardware error)
		sleep(1);
		
	}
	//modbus_set_debug(mb, TRUE);

	ret = 1;	
	
	while(ret || ret == -1){
		ret = modbus_set_slave(mb, wstw_cfg.slv_adr);
		//set anemometer status code (software error)
		sleep(1);
	}

	ret = 1;

	while(ret || ret == -1){
		ret = modbus_connect(mb);
		//set anemometer status code (anemometer comm error);
		sleep(1);
	}

	ret = 0;

	//read wind speed from sensor register

	spd_buf = malloc(wstw_cfg.spd_buf_len*sizeof(float));

	for(i=0; i<wstw_cfg.spd_buf_len; i++){
		*(spd_buf+i) = 0;
	}

	st = chksum_cfg = chksum_stat = 0;

	chksum_cfg += 100*wstw_cfg.spd_lim;
	chksum_cfg += 100*wstw_cfg.stow_fac;
	chksum_cfg += 100*wstw_cfg.unstow_fac;
	chksum_cfg += 100*wstw_cfg.crnt_lim;
	chksum_cfg += 100*wstw_cfg.stow_del;
	chksum_cfg += 100*wstw_cfg.unstow_del;
	chksum_cfg += 100*wstw_cfg.avg_len;
	chksum_cfg += 100*wstw_cfg.sstow_fac;
	chksum_cfg += 100*wstw_cfg.sustow_fac;

	//added to modbus config
	//configuration updated in realtime
	//remaining require software restart
	wstw_cfg.chksum += 100*wstw_cfg.spd_buf_len;



	for(;;){


		ret = modbus_read_input_registers(mb, wstw_cfg.reg_adr, wstw_cfg.reg_num, tab_reg16);

		if(ret == -1){
			*tab_reg16 = 0;
			//set sensor status code (anemometer comm error)
			set_ctrl_fault("SNSR", 0, 1);
			//printf("\nFailed to read anemometer register: %s\n\n", modbus_strerror(errno));
		}
		else if(ret>0 && *tab_reg16 < SENS_BLOCK){
			//set sensor status code (functional and reading)
			set_ctrl_fault("SNSR", 0, 0);
		}


		if(*tab_reg16 > SENS_BLOCK){
			*spd_buf = 0;
			set_ctrl_fault("SNSR", 1, 1);
			
			//set sensor status error code
		}
		else{
			*spd_buf = *tab_reg16/10.0;
			set_ctrl_fault("SNSR", 1, 0);
		}

		run_avg_spd = avg(spd_buf, wstw_cfg.avg_len);



		/*********************** wind stow algorithm *************************************/
		if(run_avg_spd >= wstw_cfg.spd_lim/wstw_cfg.stow_fac && !stw_cmd){
			
			st_del++;

			if (st_del >= wstw_cfg.stow_del){

				stw_cmd = 1;

				st_del = 0;

			}	
		}
		else if(stw_stat.frcst_spd >= wstw_cfg.spd_lim/wstw_cfg.stow_fac && !stw_cmd){

			stw_cmd = 1;
		}
		else if(run_avg_spd < wstw_cfg.spd_lim/wstw_cfg.unstow_fac && stw_stat.frcst_spd < wstw_cfg.spd_lim/wstw_cfg.unstow_fac && stw_cmd){

			ust_del++;

			if(ust_del >= wstw_cfg.unstow_del){

					stw_cmd = 0;

					ust_del = 0;
			}
		}
		else if (run_avg_spd >= wstw_cfg.spd_lim/wstw_cfg.stow_fac || run_avg_spd < wstw_cfg.spd_lim/wstw_cfg.unstow_fac && (st_del || ust_del))
			st_del = ust_del = 0;

		shift_buf(spd_buf, wstw_cfg.spd_buf_len);

		update_stw_stat(spd_buf, run_avg_spd, stw_cmd);

		i++;
		if(i==10){
			i = 0;
			sens_stat = sensor_stat(mb);
		}
		sleep(1);
		/*************************** end wind stow algorithm *******************************/
	}
}


static int update_stw_stat(float *spd, double avg_spd, int st){

	int ret;

	ret = pthread_mutex_trylock(&lock_stat);

	if(!ret){
		stw_stat.w_stow = st;
		stw_stat.spd = *spd;
		stw_stat.avg_spd = avg_spd;

		pthread_mutex_unlock(&lock_stat);
	}

	return(ret);
}

static void shift_buf(float *spd_buf, int spd_buf_len){

	int i;

	for(i=spd_buf_len; i>0; i--)
		*(spd_buf+i) = *(spd_buf+i-1);
}

static float avg(float *spd_buf, int avg_len){

	int i;
	float sum;

	sum = 0;

	for(i=0; i<avg_len; i++)
		sum += *(spd_buf+i);

	return(sum/avg_len);
}

static int sensor_stat(modbus_t *mb){

	int ret;
	uint16_t *stat0, *stat1;

	stat0 = malloc(sizeof(int));
	stat1 = malloc(sizeof(int));

	ret = modbus_read_input_registers(mb, 2, 1, stat0);

	ret = modbus_read_input_registers(mb, 3, 1, stat1);


	//finish this

	return(*stat0);

}
