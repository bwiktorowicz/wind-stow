#include "stow_controller.h"

#define BASE_ADDR 0xe8000000

int init_rs485(void){

	unsigned int *rs, *start;
	int fd_485;
	
	fd_485 = open("/dev/mem", O_RDWR|O_SYNC);

	if(fd_485 == -1){
		printf("RS485_INIT - Failed to access memory location: %s\n\n", strerror(errno));
		return -1;
	}

	start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd_485, BASE_ADDR);
	rs = (unsigned int *)(start + 0xc);
	*rs = (*rs & 0xffff7fff);
	munmap((void*)rs, getpagesize());
	close(fd_485);

	return 1;
}
