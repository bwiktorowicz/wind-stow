#include <math.h>
#include "stow_controller.h"

#define CTRL_STAT 8
#define SENSE_STAT 8

int set_ctrl_fault(char *fault, int bit, int stat){

	int *stat_b, stat_d, fault_len, *fault_ptr;

	if(!strcmp(fault, "CTRL")){
		fault_len = CTRL_STAT;
		fault_ptr = &stw_stat.ctrl_fault;
	}
	else if(!strcmp(fault, "SNSR")){
		fault_len = SENSE_STAT;
		fault_ptr = &stw_stat.snsr_fault;
	}

	stat_b = dtob(*fault_ptr, fault_len);

	*(stat_b+bit) = stat;

	stat_d = btod(stat_b, fault_len);

	pthread_mutex_lock(&lock_stat);

	*fault_ptr = stat_d;
	
	pthread_mutex_unlock(&lock_stat);

	return(*fault_ptr);
}

int *dtob(int code, int len){


	int i, j, *bin;

	bin = malloc(len*sizeof(int));

	for(i=0; i<len; i++)
		*(bin+i) = 0;

	i = 1;

	*bin = code%2;

	while((int)code/2){
		code = (int)code/2;
		*(bin+i) = code%2;
		i++;
	}

	j = i-1;

	return(bin);
}

int btod(int *bin, int len){

	int i, ret;

	ret = 0;

	for(i=0; i<len; i++)
		ret = ret + *(bin+i)*pow(2,i);

	return(ret);
}
