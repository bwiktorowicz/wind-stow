#include "stow_controller.h"
#define iface_cfg "/etc/network/interfaces.test"

#define loop_iface "auto lo\niface lo inet loopback\n\n"
#define ethx_iface1 "auto "
#define ethx_iface2 "iface "
#define ethx_iface3 "inet static "
#define ethx_iface4 "\taddress "
#define ethx_iface5 "\tnetmask "
#define ethx_iface6 "\tgateway "

#define dhcp_iface "inet dhcp\n"

#define null_adr "0.0.0.0"

#define BUFF_LEN 100

int net_cfg(char *mode, char *ipadr, char *nmask, char *gtway, char *iface){

	int i;
	char *line;
	FILE *pp, *fd;
	

	line = malloc(BUFF_LEN*sizeof(char));

	fd = fopen(iface_cfg, "wb");

	fprintf(fd, "%s", loop_iface);
	fprintf(fd, "%s%s\n", ethx_iface1, iface);

	pthread_mutex_lock(&lock_ctr);

	if(!strcmp(mode, "static")){
		printf("%s\n", gtway);
		fprintf(fd, "%s%s %s\n", ethx_iface2, iface, ethx_iface3);
		fprintf(fd, "%s%s\n", ethx_iface4, ipadr);
		fprintf(fd, "%s%s\n", ethx_iface5, nmask);
		fprintf(fd, "%s%s\n", ethx_iface6, gtway);

		tctr_cfg.stcp_adr = ipadr;
		tctr_cfg.stcp_nmask = nmask;
		tctr_cfg.stcp_gtway = gtway;
	}
	else if(!strcmp(mode, "dhcp")){
		fprintf(fd, "%s%s %s", ethx_iface2, iface, dhcp_iface);

		tctr_cfg.stcp_adr = null_adr;
		tctr_cfg.stcp_nmask = nmask;
		tctr_cfg.stcp_gtway = gtway;
	}

	fclose(fd);

	pthread_mutex_unlock(&lock_ctr);

	pp = popen("ifdown eth0 2>&1 && ifup eth0 2>&1", "r");

	printf("\n\n\r");
	i = 0;

	if(pp != NULL){
			
		while(fgets(line, BUFF_LEN, pp)!=NULL)
			printf("%s\r", line);

		printf("\n");
		
		pclose(pp);
	}
	else{
		printf("could not bring down interface\n");
		return(0);
	}

	return(1);
}
