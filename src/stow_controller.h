#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/serial.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <termios.h>
#include <modbus/modbus.h>
#include <sys/mman.h>
#include <fcntl.h>

//#define clear() printf("\033[H\033[J")

#define BASE_ADDR 0xe8000000
#define STAT_LEN 9
#define NUM_THREADS 6
#define STR_BUF 32

#define NETMSK "255.255.255.0"

#define CONFIG_FIL "/usr/local/share/stow_ctr/stwctr.cfg"
#define ADDR_FIL "/usr/local/share/stow_ctr/addr.cfg"

//global
//structure holding values to read from anemometer and run stowing algorithm

typedef struct _stw_cfg{
	int slv_adr;		//sensor slave address
	int reg_adr;		//sensor register address
	int reg_num;		//number of registers to read
	int spd_buf_len;	//length of buffer holding wind speeds
	int avg_len;		//length of running average
	int stow_del;		//length of delay for stow
	int unstow_del;		//length of delay for unstow
	int stow_ovr;		//overide stow command status
	float stow_fac;		//stow factor
	float unstow_fac;	//unstow factor
	float spd_lim;		//critical wind speed limit [m/s]
	float crnt_lim;		//critical current limit [A]
	int frcst_delt;		//forecast wind speed lookahead [hr]
	float sstow_fac;	//snow stow factor
	float sustow_fac;	//snow unstow factor
	int chksum;
}stw_cfg;

//tracker ctrl status structure

typedef struct _ctr_cfg{
	int slv_adr;		//tracker controller slave address
	int stow_reg_adr;	//tracker controller stow command register address
	int stat_reg_adr;	//tracker controller stow status register address
	int reg_num;		//number of registers to read/write
	int ctr_num;		//number of controllers
	int port;		//tcp port of tracker controllers
	int ctrl_rst;		//controller thread reset
	char **tcp_adr;		//array holding all tracker controller IP addresses
	char *stcp_adr;		//stow controller ip address
	char *stcp_nmask;	//stow controller netmask
	char *stcp_gtway;	//stow controller gateway
	int *ctr_stat;		//array holding status for each tracker controller
	int chksum;
}ctr_cfg;

//stow ctrl status structure
typedef struct _ctrl_stt{
	int w_stow;		//wind stow command
	int s_stow;		//snow stow command
	int *tw_stow;		//tracker controller wind stow status
	int *ts_stow;		//traker controller snow stow status
	float spd;		//wind speed
	int frcst_spd;		//institute predicted wind speed
	int crnt;		//motor current
	double avg_spd;		//average wind speed
	int avg_crnt;		//average current
	int snsr_fault;		//sensor status code
	int ctrl_fault;		//stow controller status code
	float lat;		//stow controller latitude
	float lon;		//stow controller longitude
	time_t gps_time;	//current time reported by Controller GPS receiver
	int tm_zone;		//local time zone
	int chksum;		
}ctrl_stt;

/**********Fault List***********

bit	description

CTRL
0	Controller not responding
1	stw command sent but Controller not stowing
2	stw command received but Controller not stowing
3	current transducer error
4	NWS/GFS error
5	GPS data not available
6
7
8


SNSR
0	anemometer comm error
1	anemometer blocked
2
3
4
5
6
7


*******************************/



stw_cfg wstw_cfg;
pthread_mutex_t lock_stw;

ctr_cfg tctr_cfg;
pthread_mutex_t lock_ctr;

ctrl_stt stw_stat;
pthread_mutex_t lock_stat;

int stw_cmd;

//structures holding command usage information

typedef struct _help_dat{
	int num;
	char **cmd;
	char **desc;
	char **use;
}help_dat;

int init_rs485(void);
void *wind_monitor(void *arg);
void *stw_ctrl(void *arg);
int poll_ctr(ctr_cfg tctr_cfg);
//int *ctrl_stat(char *addr);
int set_ctr(char *addr, int cmd);
int *dtob(int stat, int len);
int btod(int *bin, int len);
int init_ctrl(void);
void *modbus_srv(void *arg);
void *init_iface(void *arg);
static void fin(int sig);
int write_cfg(char *cmd, char *val);
int net_cfg(char *mode, char *ipadr, char *nmap, char *gtway, char *iface);
void *forcst_dat(void *arg);
int set_ctrl_fault(char *fault, int bit, int stat);
void *ctrl_util(void *arg);
int comb_byte(int minor, int major);
int get_ctrl_adr(char *name);


//*tctr_cfg.stcp_adr = "0.0.0.0";
//*tctr_cfg.stcp_nmask = "0.0.0.0";
//*tctr_cfg.stcp_gtway = "0.0.0.0";
