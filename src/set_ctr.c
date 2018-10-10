#include "stow_controller.h"

int set_ctr(char *addr, int cmd){

	int i, ret;
	char **buf;
	modbus_t *mb;
	uint16_t tab_reg16[32];

	tab_reg16[0] = cmd;

//	for(i=0; i<tctr_cfg.ctr_num; i++){

		//mb = modbus_new_tcp(*(argv+1), atoi(*(argv+3)));

		mb = modbus_new_tcp(addr, tctr_cfg.port);

		modbus_set_slave(mb, tctr_cfg.slv_adr);

		if(modbus_connect(mb) == -1){
//			printf("\nController write connection failed: %s\n\n", modbus_strerror(errno));
			modbus_free(mb);
			return -1;
		}

		//printf("Writing to Controller register(s)\n");
		ret = modbus_write_registers(mb, tctr_cfg.stow_reg_adr, tctr_cfg.reg_num, tab_reg16);

		if(ret == -1){
//			printf("\nFailed to write Controller registers: %s\n\n", modbus_strerror(errno));
			modbus_close(mb);
			modbus_free(mb);
			return -1;
		}


		modbus_close(mb);
		modbus_free(mb);
//	}
}
