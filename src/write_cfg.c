#include "stow_controller.h"

int write_cfg(char *cmd, char *val){

	int i, j, sz;
	char *cfg_val;
	char *tmp, **cfg_key;
	FILE *fp;


	fp = fopen(CONFIG_FIL, "r+");

	while(!fscanf(fp, "%*[^\n]%*c"))
		sz++;
	fseek(fp, 0L, SEEK_SET);
	tmp = malloc(STR_BUF*sizeof(char));
	cfg_val = malloc(STR_BUF*sizeof(char));


	i = 0;

	while(fscanf(fp, "%s %s", tmp, cfg_val) != EOF){
		if(!strcmp(tmp, cmd)){
			fseek(fp, -(strlen(tmp)+strlen(cfg_val)+1), SEEK_CUR);
			for(j=0; j<(strlen(tmp)+strlen(cfg_val)+1); j++)
				fprintf(fp, " ");
			fseek(fp, -(strlen(tmp)+strlen(cfg_val)+1), SEEK_CUR);
			fprintf(fp, "%s %.3f\n", tmp, atof(val));
			break;
		}
	}

	fclose(fp);

	pthread_mutex_lock(&lock_stw);
//	pthread_mutex_lock(&lock_ctr);

	if(!strcmp(cmd, "spd_lim"))
		wstw_cfg.spd_lim = atof(val);
	else if(!strcmp(cmd, "stow_fac"))
		wstw_cfg.stow_fac = atof(val);
	else if(!strcmp(cmd, "unstow_fac"))
		wstw_cfg.unstow_fac = atof(val);
	else if(!strcmp(cmd, "stow_del"))
		wstw_cfg.stow_del = atoi(val);
	else if(!strcmp(cmd, "unstow_del"))
		wstw_cfg.unstow_del = atoi(val);
	else if(!strcmp(cmd, "spd_buf_len"))
		wstw_cfg.spd_buf_len = atoi(val);
	else if(!strcmp(cmd, "avg_len"))
		wstw_cfg.avg_len = atoi(val);
	else if(!strcmp(cmd, "sstow_fac"))
		wstw_cfg.sstow_fac = atof(val);
	else if(!strcmp(cmd, "sustow_fac"))
		wstw_cfg.sustow_fac = atof(val);
	else if(!strcmp(cmd, "crnt_lim"))
		wstw_cfg.crnt_lim = atof(val);
	else if(!strcmp(cmd, "sasa"))
		wstw_cfg.slv_adr = atoi(val);
	else if(!strcmp(cmd, "sara"))
		wstw_cfg.reg_adr = atoi(val);
	else if(!strcmp(cmd, "sarn"))
		wstw_cfg.reg_num = atoi(val);
	else if(!strcmp(cmd, "stsa"))
		tctr_cfg.slv_adr = atoi(val);
	else if(!strcmp(cmd, "stsp"))
		tctr_cfg.port = atoi(val);
	else if(!strcmp(cmd, "stsr"))
		tctr_cfg.stow_reg_adr = atoi(val);
	else if(!strcmp(cmd, "stsn"))
		tctr_cfg.reg_num = atoi(val);
	else if(!strcmp(cmd, "sstatr"))
		tctr_cfg.stat_reg_adr = atoi(val);
	else if(!strcmp(cmd, "sra"))
		write_addr(val);



	wstw_cfg.chksum = 0;

	wstw_cfg.chksum += 100*wstw_cfg.spd_lim;
	wstw_cfg.chksum += 100*wstw_cfg.stow_fac;
	wstw_cfg.chksum += 100*wstw_cfg.unstow_fac;
	wstw_cfg.chksum += 100*wstw_cfg.crnt_lim;
	wstw_cfg.chksum += 100*wstw_cfg.stow_del;
	wstw_cfg.chksum += 100*wstw_cfg.unstow_del;
	wstw_cfg.chksum += 100*wstw_cfg.avg_len;
	wstw_cfg.chksum += 100*wstw_cfg.sstow_fac;
	wstw_cfg.chksum += 100*wstw_cfg.sustow_fac;

//	wstw_cfg.chksum += 100*wstw_cfg.spd_buf_len;

/*delete
	wstw_cfg.chksum += 100*wstw_cfg.slv_adr;
	wstw_cfg.chksum += 100*wstw_cfg.reg_adr;
	wstw_cfg.chksum += 100*wstw_cfg.reg_num;

	tctr_cfg.chksum = 0;

	tctr_cfg.chksum += 100*tctr_cfg.slv_adr;
	tctr_cfg.chksum += 100*tctr_cfg.port;
	tctr_cfg.chksum += 100*tctr_cfg.stow_reg_adr;
	tctr_cfg.chksum += 100*tctr_cfg.reg_num;
	tctr_cfg.chksum += 100*tctr_cfg.stat_reg_adr;
	*/
	

	pthread_mutex_unlock(&lock_stw);
//	pthread_mutex_unlock(&lock_ctr);

	free(tmp);
	free(cfg_val);

	return(1);
}

int write_addr(char *fil){

	FILE *fp;

	fp = fopen(ADDR_FIL, "w");

}
