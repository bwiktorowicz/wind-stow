#include "stow_controller.h"

#define IP_ADR_LEN 16
#define TMP_ADDR_DIR "/home/jail/home/admin/"

int get_ctrl_adr(char *name){



	int i, sz;
	char *fil_pth;
	FILE *fp, *fp2;

	sz = 0;
	fil_pth = malloc(100*sizeof(char));

	*fil_pth = '\0';

	strncat(fil_pth, TMP_ADDR_DIR, sizeof(TMP_ADDR_DIR));
	strncat(fil_pth, name, strlen(name));

	fp = fopen(fil_pth, "r");

	if(!fp)
		return(-1);
	

	while(!fscanf(fp, "%*[^\n]%*c"))
		sz++;
	fseek(fp, 0L, SEEK_SET);

	pthread_mutex_lock(&lock_ctr);


	//resize allocated memory holding IP addresses


	if(sz > tctr_cfg.ctr_num){

		tctr_cfg.tcp_adr = realloc(tctr_cfg.tcp_adr, sz);

		for(i=tctr_cfg.ctr_num; i<sz; i++)
			*(tctr_cfg.tcp_adr+i) = malloc(IP_ADR_LEN*sizeof(char));
	}
	else if(sz < tctr_cfg.ctr_num){

		for(i=tctr_cfg.ctr_num-1; i>sz-1; i--)
			free(*(tctr_cfg.tcp_adr+i));

		tctr_cfg.tcp_adr = realloc(tctr_cfg.tcp_adr, sz);
	}

	//exit(0);
	
	tctr_cfg.ctr_num = sz;

	pthread_mutex_unlock(&lock_ctr);

	fp2 = fopen(ADDR_FIL, "w");

	i = 0;

	while(fscanf(fp, "%s", *(tctr_cfg.tcp_adr+i)) != EOF){
		fprintf(fp2, "%s\n", *(tctr_cfg.tcp_adr+i));
		i++;
	}

//	free(fil_pth);

	fclose(fp);
	fclose(fp2);

	pthread_mutex_lock(&lock_ctr);
	tctr_cfg.ctrl_rst = 1;
	pthread_mutex_unlock(&lock_ctr);

	return(0);
}
