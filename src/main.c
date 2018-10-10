#include "stow_controller.h"
#include <arpa/inet.h>
#include <net/if.h>

#define IP_ADR_LEN 16

main(int argc, char **argv){

	int i, ret, fd_485, sz;
	unsigned int *rs, *start;
	float *cfg_val;
	char **buf, **cfg_key, *tmp, *name;
	modbus_t *mb;
	uint16_t tab_reg16[32], tab_reg16_2[32];
	uint8_t tab_reg8[32];
	pid_t pid, sid;
	FILE *fp;

	name = "eth0";

	//check command line inputs
	if(argc != 4){
		printf("\nUsage: %s [Slave Address] [Register Address] [Number of Registers]\n\n", *(argv));
		return -1;
	}


//	net_cfg("dhcp", "555.555.555.555", "777.777.777.777", "666.666.666.666", name);


	//open and read in data from config file

	sz = 0;

	fp = fopen(CONFIG_FIL, "r");

	while(!fscanf(fp, "%*[^\n]%*c"))
                sz++;
	fseek(fp, 0L, SEEK_SET);
	tmp = malloc(STR_BUF*sizeof(char));
	cfg_key = malloc(sz*sizeof(cfg_key));
	for(i=0; i<sz; i++){
		*(cfg_key+i) = malloc(STR_BUF*sizeof(char));
	}

	cfg_val = malloc(sz*sizeof(float));

	i = 0;

	while(fscanf(fp, "%s %f", tmp, cfg_val+i) != EOF){
		strcpy(*(cfg_key+i), tmp);
		i++;
	}

	fclose(fp);

	//parse config values to initialize variables

	for(i=0; i<sz; i++){
		if(!strcmp(*(cfg_key+i), "anem_slv_adr"))
			wstw_cfg.slv_adr = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "anem_spd_reg_adr"))
			wstw_cfg.reg_adr = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "anem_spd_reg_num"))
			wstw_cfg.reg_num = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "spd_buf"))
			wstw_cfg.spd_buf_len = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i),  "stow_del"))
			wstw_cfg.stow_del = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i),  "unstow_del"))
			wstw_cfg.unstow_del = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i),  "stow_fac"))
			wstw_cfg.stow_fac = *(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "unstow_fac"))
			wstw_cfg.unstow_fac = *(cfg_val+i);
		else if(!strcmp(*(cfg_key+i),  "avg_len"))
			wstw_cfg.avg_len = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "spd_lim"))
			wstw_cfg.spd_lim = *(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "crnt_lim"))
			wstw_cfg.crnt_lim = *(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "frcst_delt"))
			wstw_cfg.frcst_delt = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "sstow_fac"))
			wstw_cfg.sstow_fac = *(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "sustow_fac"))
			wstw_cfg.sustow_fac = *(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "ctr_slv_adr"))
			tctr_cfg.slv_adr = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "stow_reg_adr"))
			tctr_cfg.stow_reg_adr = (int)*(cfg_val+i)-1;
		else if(!strcmp(*(cfg_key+i), "stat_reg_adr"))
			tctr_cfg.stat_reg_adr = (int)*(cfg_val+i)-1;
		else if(!strcmp(*(cfg_key+i), "ctr_reg_num"))
			tctr_cfg.reg_num = (int)*(cfg_val+i);
		else if(!strcmp(*(cfg_key+i), "ctr_port"))
			tctr_cfg.port = (int)*(cfg_val+i);
	}

	free(cfg_key);
	free(cfg_val);

	//open and read in data from IP address file

	sz = 0;

	fp = fopen(ADDR_FIL, "r");

	while(!fscanf(fp, "%*[^\n]%*c"))
                sz++;
	fseek(fp, 0L, SEEK_SET);

	tctr_cfg.ctr_num = sz;


	tmp = realloc(tmp, IP_ADR_LEN*sizeof(char));
	if(sz){
		tctr_cfg.tcp_adr = malloc(sz*sizeof(tctr_cfg.tcp_adr));

		i = 0;

		while(fscanf(fp, "%s", tmp) != EOF){
			*(tctr_cfg.tcp_adr+i) = malloc(IP_ADR_LEN*sizeof(char));
			strcpy(*(tctr_cfg.tcp_adr+i), tmp);
			i++;
		}
	}

	free(tmp);

	fclose(fp);
	

	//enable RS485 on the TS-7800
/*
	ret = init_rs485();
	if(ret == -1){
		printf("RS485_INIT - Call to initialize RS485 failed: %s\n\n");
		exit(EXIT_FAILURE);
	}
*/

	/***************************start the daemon process***************************/
/*
	//fork off the parent process
	pid = fork();
	if(pid < 0){
		printf("DAEMON_INIT - Failed to fork proccess: %s\n\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if(pid > 0)
		exit(EXIT_SUCCESS);

	//change file mode mask
	umask(0);

	//open logs here

	//Create new SID for child process
	sid = setsid();
	if(sid < 0){
		//log failure
		//change to write to log file
		printf("DAEMON_INIT - Failed to create new SID: %s\n\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	//change working directory
	if((chdir("/")) < 0){
		//log failure
		//change to write to log file
		printf("DAEMON_INIT - Failed to change to root directory: %s\n\n", strerror(errno));
		exit(EXIT_FAILURE);
	}	

	//close the standard file descriptors
	//close(STDIN_FILENO);
	//close(STDOUT_FILENO);
	//close(STDERR_FILENO);

	ret = 0;

	//Daemon process loop
	//while(1){

		//sleep(1);
*/
		init_ctrl();
	//}

	exit(EXIT_SUCCESS);
}
