#include "stow_controller.h"

int init_ctrl(void){

	int i, ret;

	pthread_t thr[NUM_THREADS];

	pthread_mutex_init(&lock_stw, NULL);
	pthread_mutex_init(&lock_ctr, NULL);
	pthread_mutex_init(&lock_stat, NULL);

	//stw_stat.tw_stow = malloc(tctr_cfg.ctr_num*sizeof(int));
//	tctr_cfg.ctr_stat = malloc(tctr_cfg.ctr_num*sizeof(int));

//	for(i=0; i<tctr_cfg.ctr_num; i++)
//		*(tctr_cfg.ctr_stat+i) = 0;


	/**********Replace these with utility function**************/
//	stw_stat.tm_zone = -7;



	//wind data acquisition and stow algorithm thread
	ret = pthread_create(&thr[0], NULL, &wind_monitor, NULL);
	if(ret){
		//handle error
		;
		return EXIT_FAILURE;
	}


	//Controller stow controller thread
	ret = pthread_create(&thr[1], NULL, &stw_ctrl, NULL);
	if(ret){
		//handle error
		;
		return EXIT_FAILURE;
	}

	sleep(2);

	//modbus server thread
	ret = pthread_create(&thr[2], NULL, &modbus_srv, NULL);
	if(ret){
		//handle error
		;
		printf("failed\n");
		return EXIT_FAILURE;
	}

	//Wind speed prediction data retreival thread
	ret = pthread_create(&thr[3], NULL, &forcst_dat, NULL);
	if(ret){
		//handle error
		;
		return(EXIT_FAILURE);
	}

	//Controller maintenance thread
	ret = pthread_create(&thr[4], NULL, &ctrl_util, NULL);
	if(ret){
		//handle error
		;
		return(EXIT_FAILURE);
	}

	//configuration interface
	ret = pthread_create(&thr[5], NULL, &init_iface, NULL);
	if(ret){
		//handle error
		;
		return EXIT_FAILURE;
	}

	for(i=0; i<NUM_THREADS; i++)
		pthread_join(thr[i], NULL);

	return EXIT_SUCCESS;

}
