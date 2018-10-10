#include <time.h>
#include <curses.h>
#include <signal.h>
#include <wchar.h>
#include <locale.h>
#include <sys/syscall.h>
#include <errno.h>
#include "stow_controller.h"

#define SEC_NUM 4
#define ALG_NUM 24
#define COMM_NUM 16
#define SYS_NUM 6
#define INIT_X 2
#define INIT_Y 1

#define BUFF_LEN 100

help_dat sec_help, alg_help, comm_help, sys_help;

//R03u570 config interface

static char *search_help(char *cmd);

void *init_iface(void *arg){

	int cr, i, row, col;
	char c, *buf;
	WINDOW *main_win;

	buf = malloc(256*sizeof(char));

	gen_help_men();

	(void)signal(SIGINT, fin);
	(void)initscr();
	keypad(stdscr, TRUE);
	//(void) nonl();
	(void)raw();
	(void)noecho();
	scrollok(stdscr, TRUE);
	sleep(1);
	clear();

	if (has_colors()){

		start_color();

		init_pair(1, COLOR_RED,     COLOR_BLACK);
		init_pair(2, COLOR_GREEN,   COLOR_BLACK);
		init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
		init_pair(4, COLOR_BLUE,    COLOR_BLACK);
		init_pair(5, COLOR_CYAN,    COLOR_BLACK);
		init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(7, COLOR_WHITE,   COLOR_BLACK);
	}

	i = c = 0;

	prompt();

	while(c !=31){

		cr = getch();
		c = (char)cr;

		if(c == 10 && i){
			*(buf+i) = '\0';
			interp(buf, i);
			i = 0;
			prompt();
		}
		else if(c == 7 && i>=1){
			i--;
			getyx(stdscr, row, col);
			mvwdelch(stdscr, row, col-1);
		}
		else if(c != 7 && c != 10){
			*(buf+i) = c;
			addch(c);
			i++;
		}
		else if(c == 10 && !i){
			addch(c);
			prompt();
		}

	}

	fin(0);
}

static void fin(int sig){

	endwin();
	exit(0);
}

int prompt(void){

	init_pair(1, COLOR_RED,     COLOR_BLACK);
	init_pair(7, COLOR_WHITE,   COLOR_BLACK);

	attron(COLOR_PAIR(1));
	printw("%s", "RobuStow");
	attron(COLOR_PAIR(7));
	printw("%s", " # ");
}

int interp(char *buf, int num){

	int i, rc, ret, row, col;
	char *token, **cmd, *s, *line;
	FILE *pp;

	s = malloc(2*sizeof(*cmd));
	cmd = malloc(5*sizeof(*cmd));
	for(i=0; i<5; i++)
		*(cmd+i) = malloc(30*sizeof(char));

	s = " ";
	i=0;


	token = strtok(buf, s);
	do{
		strcpy(*(cmd+i), token);
		token = strtok(NULL, s);
		i++;
	}while(token != NULL && i<5);

	getyx(stdscr, row, col);
	wmove(stdscr, row, col);

	i--;

	if(!strcmp(*cmd, "help"))
		help_menu(cmd, i);
	else if(!strcmp(*cmd, "dwst"))
		printw("\n%.3f\n", wstw_cfg.spd_lim/wstw_cfg.stow_fac);
	else if(!strcmp(*cmd, "dwut"))
		printw("\n%.3f\n", wstw_cfg.spd_lim/wstw_cfg.unstow_fac);
	else if(!strcmp(*cmd, "dwsl"))
		printw("\n%.3f\n", wstw_cfg.spd_lim);
	else if(!strcmp(*cmd, "dwsf"))
		printw("\n%.3f\n", wstw_cfg.stow_fac);
	else if(!strcmp(*cmd, "dwuf"))
		printw("\n%.3f\n", wstw_cfg.unstow_fac);
	else if(!strcmp(*cmd, "dwsd"))
		printw("\n%d\n", wstw_cfg.stow_del);
	else if(!strcmp(*cmd, "dwud"))
		printw("\n%d\n", wstw_cfg.unstow_del);
	else if(!strcmp(*cmd, "dabuf"))
		printw("\n%d\n", wstw_cfg.spd_buf_len);
	else if(!strcmp(*cmd, "dral"))
		printw("\n%d\n", wstw_cfg.avg_len);
	else if(!strcmp(*cmd, "dsst"))
		printw("\n%.3f\n", wstw_cfg.crnt_lim/wstw_cfg.sstow_fac);
	else if(!strcmp(*cmd, "dsut"))
		printw("\n%.3f\n", wstw_cfg.crnt_lim/wstw_cfg.sustow_fac);
	else if(!strcmp(*cmd, "dscl"))
		printw("\n%.3f\n", wstw_cfg.crnt_lim);
	else if(!strcmp(*cmd, "dssf"))
		printw("\n%3f\n", wstw_cfg.sstow_fac);
	else if(!strcmp(*cmd, "dsuf"))
		printw("\n%3f\n", wstw_cfg.sustow_fac);
	else if(!strcmp(*cmd, "dasa"))
		printw("\n%d\n", wstw_cfg.slv_adr);
	else if(!strcmp(*cmd, "dara"))
		printw("\n%d\n", wstw_cfg.reg_adr);
	else if(!strcmp(*cmd, "darn"))
		printw("\n%d\n", wstw_cfg.reg_num);
	else if(!strcmp(*cmd, "dtsa"))
		printw("\n%d\n", tctr_cfg.slv_adr);
	else if(!strcmp(*cmd, "dtsp"))
		printw("\n%d\n", tctr_cfg.port);
	else if(!strcmp(*cmd, "dtsr"))
		printw("\n%d\n", tctr_cfg.stow_reg_adr);
	else if(!strcmp(*cmd, "dtsn"))
		printw("\n%d\n", tctr_cfg.reg_num);
	else if(!strcmp(*cmd, "dstatr"))
		printw("\n%d\n", tctr_cfg.stat_reg_adr);
	else if(!strcmp(*cmd, "dra"))
		printw("\n%s\n", tctr_cfg.stcp_adr); 
	else if(!strcmp(*cmd, "dtn"))
		printw("\n%d\n", tctr_cfg.ctr_num);
	else if(!strcmp(*cmd, "dtas")){
			printw("\n");
		pthread_mutex_lock(&lock_ctr);
		for(i=0; i<tctr_cfg.ctr_num; i++)
			printw("%d %s\n", i, *(tctr_cfg.tcp_adr+i));
		pthread_mutex_unlock(&lock_ctr);
	}
	else if(!strcmp(*cmd, "dtc")){
		printw("\nLongitude:\t%f", stw_stat.lon);
		addch(ACS_DEGREE);
		printw("\nLatitude:\t%f", stw_stat.lat);
		addch(ACS_DEGREE);
		printw("\n");
	}
	else if(!strcmp(*cmd, "swsl")){
		write_cfg("spd_lim", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "swsf")){
		write_cfg("stow_fac", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "swuf")){
		write_cfg("unstow_fac", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "swsd")){
		write_cfg("stow_del", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "swud")){
		write_cfg("unstow_del", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "sabuf")){
		write_cfg("spd_buf_len", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "sral")){
		write_cfg("avg_len", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "sssf")){
		write_cfg("sstow_fac", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "ssuf")){
		write_cfg("sustow_fac", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "sccl")){
		write_cfg("crnt_lim", *(cmd+1));
		printw("\n");
	}
/******************/
	else if(!strcmp(*cmd, "sasa")){
		write_cfg("slv_adr", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "sara")){
		write_cfg("reg_adr", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "sarn")){
		write_cfg("reg_num", *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "stsa")){
		write_cfg(*cmd, *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "stsp")){
		write_cfg(*cmd, *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "stsr")){
		write_cfg(*cmd, *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "stsn")){
		write_cfg(*cmd, *(cmd+1));
		printw("\n");
	}
	else if(!strcmp(*cmd, "sstatr")){
		write_cfg(*cmd, *(cmd+1));
		printw("\n");
	}
/*****************/
	else if(!strcmp(*cmd, "stat"))
		ctrl_status();
	else if(!strcmp(*cmd, "ripa")){
		if(get_ctrl_adr(*(cmd+1))<0)
			printw("\nError opening file: %s\n", *(cmd+1));
	}
	else if(!strcmp(*cmd, "sra")){
		if(!strcmp(*(cmd+1), "static")){
			ret = net_cfg("static", *(cmd+2), *(cmd+3), *(cmd+4), "eth0");
			printw("%s\n", *(cmd+3));
		}
		else if(!strcmp(*(cmd+1), "dhcp")){
			ret = net_cfg("dhcp", "NULL", "NULL", "NULL", "eth0");
			if(!ret)
				printw("\nFailed to reinitialize network interface\n");
		}		
		else{
			printw("\nCommand syntax error\n");
			printw("%s:\t\t%s\n", *(sys_help.cmd+3), *(sys_help.use+3));
		}
	}
	else if(!strcmp(*cmd, "stz")){
		//rc = system("dpkg-reconfigure tzdata");
		//rc = system("tzselect");
		pp = popen("tzselect", "r");
		printw("\n\n\t");
		i = 0;
		if(pp != NULL){
			while(fgets(line, BUFF_LEN, pp)!=NULL)
				printw("%s\r", line);

			printw("\n");
			pclose(pp);
		}
//		if(rc == -1)
//			printw("\nError starting timezone configuration\n");
	}
	else
		printw("\nInvalid command: %s\n", *cmd);

}

int ctrl_status(void){

	int i, c, init_x, init_y;
	char *frmt_tm;
	WINDOW *win;
	time_t c_tm;
	struct tm *cur_tm;

	timeout(100);
	curs_set(0);

	init_x = INIT_X;
	init_y = INIT_Y;

	
	frmt_tm = malloc(25*sizeof(char));

	clear();
	win = newwin(14+tctr_cfg.ctr_num,58,0,0);

	while(getch()==-1){

		c_tm = time(NULL);
		cur_tm = localtime(&c_tm);
		frmt_tm = asctime(cur_tm);
//		*(frmt_tm+strlen(frmt_tm)-1) = '\0';
		wmove(win, init_y, init_x);
		wprintw(win, "System Date/Time: \t\t%s", frmt_tm);
		wclrtoeol(win);
		wmove(win, init_y+=2, init_x);
		wprintw(win, " ");
		wmove(win, init_y++, init_x);
		wprintw(win, "Anemometer Reading: \t\t%f m/s", stw_stat.spd);
		wclrtoeol(win);
		wmove(win, init_y++, init_x);
		wprintw(win, "Running Average: \t\t%f m/s", stw_stat.avg_spd);
		wclrtoeol(win);
		wmove(win, init_y++, init_x);
		wprintw(win, "Predicted Wind Speed: \t%f m/s", (float)stw_stat.frcst_spd);
		wclrtoeol(win);
		wmove(win, init_y++, init_x);
		wprintw(win, "Stow Command: \t\t%d", stw_stat.w_stow);
		wclrtoeol(win);
		wmove(win, init_y++, init_x);
		wprintw(win, "Ctrl Fault Code: \t\t%d", stw_stat.ctrl_fault);
		wclrtoeol(win);
		wmove(win, init_y++, init_x);
		wprintw(win, "Sensor Fault Code: \t\t%d", stw_stat.snsr_fault);
		wclrtoeol(win);
		wmove(win, init_y+=2, init_x);
		wprintw(win, "   Controller #\t\t Controller IP\tStatus Code");
		wclrtoeol(win);
		init_y+=2;
		for(i=0; i<tctr_cfg.ctr_num; i++){
			wmove(win, init_y++, init_x);
			wprintw(win, "\t%d\t\t%s\t     %d", i, *(tctr_cfg.tcp_adr+i), *(tctr_cfg.ctr_stat+i));
			wclrtoeol(win);
		}

		wmove(stdscr, init_y+=2, init_x);
		printw("Press any key to exit.");
		box(win, ACS_VLINE, ACS_HLINE);
		wrefresh(win);
		//doupdate();
		init_y = INIT_Y;
		sleep(.5);
	}


	clear();
	//free(frmt_tm);
	timeout(-1);
	curs_set(1);
}

int help_menu(char **cmd, int num){

	int i;
	char *help_txt;


//	printw("%s %s\n", *cmd, *(cmd+1));
	
	if(!strcmp(*cmd, "help") && !num){
		printw("\n\nATSS System Configuration\n\n");
		for(i=0; i<55; i++)
			addch(ACS_HLINE);
		//printw("===============================================\n");
		printw("\n%s\t%s\n", "Section:", "Description");
		//printw("===============================================\n");
		for(i=0; i<55; i++)
			addch(ACS_HLINE);
		
		printw("\n\n");
		for(i=0; i<sec_help.num; i++)
			printw("%s:\t\t%s\n", *(sec_help.cmd+i), *(sec_help.desc+i));
		printw("\n");
		printw("%s\n", "Usage: help <section>");
		printw("\n");
	}
	else if(!strcmp(*cmd, "help") && num){
		if(!strcmp(*(cmd+1), "alg")){
			//print algorithm cmd help
			printw("\n\nAlgorithm Configuration\n\n");
			for(i=0; i<55; i++)
				addch(ACS_HLINE);
			//printw("===============================================\n");
			printw("\n%s\t%s\n", "Command:", "Description");
			for(i=0; i<55; i++)
				addch(ACS_HLINE);
			//printw("===============================================\n");

			printw("\n\n");
			for(i=0; i<alg_help.num; i++)
				printw("%s:\t\t%s\n", *(alg_help.cmd+i), *(alg_help.desc+i));
			printw("\n");
			printw("%s\n", "Usage: help <command>");
			printw("\n");
		}
		else if(!strcmp(*(cmd+1), "comms")){
			//print comms cmd help
			printw("\n\nCommunications Configuration\n\n");
			//printw("===============================================\n");
			for(i=0; i<55; i++)
				addch(ACS_HLINE);
			printw("\n%s\t%s\n", "Command:", "Description");
			//printw("===============================================\n");
			for(i=0; i<55; i++)
				addch(ACS_HLINE);

			printw("\n\n");
			for(i=0; i<comm_help.num; i++)
				printw("%s:\t\t%s\n", *(comm_help.cmd+i), *(comm_help.desc+i));
			printw("\n");
			printw("%s\n", "Usage: help <command>");
			printw("\n");
		}
		else if(!strcmp(*(cmd+1), "sys")){
			//print net cmd help
			printw("\n\nSystem Configuration\n\n");
			for(i=0; i<55; i++)
				addch(ACS_HLINE);
			//printw("===============================================\n");
			printw("\n%s\t%s\n", "Command:", "Description");
			//printw("===============================================\n");
			for(i=0; i<55; i++)
				addch(ACS_HLINE);

			printw("\n\n");
			for(i=0; i<sys_help.num; i++)
				printw("%s:\t\t%s\n", *(sys_help.cmd+i), *(sys_help.desc+i));
			printw("\n");
			printw("%s\n", "Usage: help <command>");
			printw("\n");
		}
		else{
			help_txt = search_help(*(cmd+1));
			if(help_txt)
				printw("\n\n%s\n\n", help_txt);
			else
			printw("\n%s%s\n", "Invalid help option: ", *(cmd+1));
		}
	}
}

static char *search_help(char *cmd){

	int i;


	for(i=0; i<ALG_NUM; i++){
		if(!strcmp(cmd, *(alg_help.cmd+i))){
			return(*(alg_help.use+i));
		}
	}

	for(i=0; i<COMM_NUM; i++){
		if(!strcmp(cmd, *(comm_help.cmd+i))){
			return(*(comm_help.use+i));
		}
	}

	for(i=0; i<SYS_NUM; i++){
		if(!strcmp(cmd, *(sys_help.cmd+i))){
			return(*(sys_help.use+i));
		}
	}

	return(NULL);
}

int alloc_help_struct(help_dat *dat_struct, int nm){

	int i;

	dat_struct->num = nm;

	dat_struct->cmd = malloc(nm*sizeof(dat_struct->cmd));
	for(i=0; i<nm; i++)
		*(dat_struct->cmd+i) = malloc(80*sizeof(char));
	dat_struct->desc = malloc(nm*sizeof(dat_struct->desc));
	for(i=0; i<nm; i++)
		*(dat_struct->desc+i) = malloc(80*sizeof(char));
	dat_struct->use = malloc(nm*sizeof(dat_struct->use));
	for(i=0; i<nm; i++)
		*(dat_struct->use+i) = malloc(80*sizeof(char));
}


int gen_help_men(void){

	//allocate memory for help menu data structures

	alloc_help_struct(&sec_help, SEC_NUM);

	alloc_help_struct(&alg_help, ALG_NUM);

	alloc_help_struct(&comm_help, COMM_NUM);

	alloc_help_struct(&sys_help, SYS_NUM);

		//general help menu data
		*sec_help.cmd = "alg";
		*sec_help.desc = "Algorithm configuration";
		*sec_help.use = "";

		*(sec_help.cmd+1) = "comms";
		*(sec_help.desc+1) = "Communications configuration";
		*(sec_help.use+1) = "";

		*(sec_help.cmd+2) = "sys";
		*(sec_help.desc+2) = "System configuration";
		*(sec_help.use+2) = "";

		*(sec_help.cmd+3) = "stat";
		*(sec_help.desc+3) = "Display stow controller status information";
		*(sec_help.use+3) = "";

		//algorithm help menu data
		*alg_help.cmd = "dwst";
		*alg_help.desc = "Display Wind Stow Threshold [m/s]";
		*alg_help.use = "dwst";

		*(alg_help.cmd+1) = "dwut";
		*(alg_help.desc+1) = "Display Wind Unstow Threshold [m/s]";
		*(alg_help.use+1) = "dwut";

		*(alg_help.cmd+2) = "dwsl";
		*(alg_help.desc+2) = "Display Wind Speed Limit [m/s]";
		*(alg_help.use+2) = "dwsl";

		*(alg_help.cmd+3) = "dwsf";
		*(alg_help.desc+3) =  "Display Wind Stow Factor [1-2]";
		*(alg_help.use+3) = "dwsf";

		*(alg_help.cmd+4) = "dwuf";
		*(alg_help.desc+4) =  "Display Wind Unstow Factor [1-2]";
		*(alg_help.use+4) = "dwuf";

		*(alg_help.cmd+5) = "dwsd";
		*(alg_help.desc+5) =  "Display Wind Stow Delay [min]";
		*(alg_help.use+5) = "dwsd";

		*(alg_help.cmd+6) = "dwud";
		*(alg_help.desc+6) = "Display Wind Unstow Delay [min]";
		*(alg_help.use+6) = "dwud";

		*(alg_help.cmd+7) = "dabuf";
		*(alg_help.desc+7) = "Display Anemometer data BUFfer [sec]";
		*(alg_help.use+7) = "dabuf";

		*(alg_help.cmd+8) = "dral";
		*(alg_help.desc+8) = "Display Running Average Length [min]";
		*(alg_help.use+8) = "dral";

		*(alg_help.cmd+8) = "dssf";
		*(alg_help.desc+8) = "Display Snow Stow Factor [1-2]";
		*(alg_help.use+8) = "dssf";

		*(alg_help.cmd+9) = "dsuf";
		*(alg_help.desc+9) = "Display Snow Unstow Factor [1-2]";
		*(alg_help.use+9) = "dsuf";

		*(alg_help.cmd+10) = "dsst";
		*(alg_help.desc+10) = "Display Snow stow threshold [A]";
		*(alg_help.use+10) = "dsst";

		*(alg_help.cmd+11) = "dsut";
		*(alg_help.desc+11) = "Display Snow unstow threshold [A]";
		*(alg_help.use+11) = "dsut";

		*(alg_help.cmd+12) = "dccl";
		*(alg_help.desc+12) = "Display Critical Current Limit [A]";
		*(alg_help.use+12) = "dccl";

		*(alg_help.cmd+13) = "dtc";
		*(alg_help.desc+13) = "Display System Coordinates [Degrees]";
		*(alg_help.use+13) = "dtc";

		*(alg_help.cmd+14) = "swsf";
		*(alg_help.desc+14) = "Set Wind Stow Factor [1-2]";
		*(alg_help.use+14) = "swst [value]";

		*(alg_help.cmd+15) = "swuf";
		*(alg_help.desc+15) = "Set Wind Unstow Factor [1-2]";
		*(alg_help.use+15) = "swut [value]";

		*(alg_help.cmd+16) = "swsl";
		*(alg_help.desc+16) = "Set Wind Speed Limit [m/s]";
		*(alg_help.use+16) = "swsl [value]";

		*(alg_help.cmd+17) = "swsd";
		*(alg_help.desc+17) = "Set Wind Stow Delay [min]";
		*(alg_help.use+17) = "swsf [value]";

		*(alg_help.cmd+18) = "swud";
		*(alg_help.desc+18) = "Set Wind Unstow Delay [min]";
		*(alg_help.use+18) = "swud";

		*(alg_help.cmd+19) = "sabuf";
		*(alg_help.desc+19) = "Set Anemometer data BUFfer [sec]";
		*(alg_help.use+19) = "sabuf [value]";

		*(alg_help.cmd+20) = "sral";
		*(alg_help.desc+20) = "Set Running Average Length [sec]";
		*(alg_help.use+20) = "sral [value]";

		*(alg_help.cmd+21) = "sccl";
		*(alg_help.desc+21) = "Set Critical Current Limit [A]";
		*(alg_help.use+21) = "sccl [value]";

		*(alg_help.cmd+22) = "sssf";
		*(alg_help.desc+22) = "Set Snow stow factor [1-2]";
		*(alg_help.use+22) = "sssf [value]";

		*(alg_help.cmd+23) = "ssuf";
		*(alg_help.desc+23) =  "Set Snow unstow factor [1-2]";
		*(alg_help.use+23) = "ssuf [value]";

		//communications help menu data
		*comm_help.cmd = "dasa";
		*comm_help.desc =  "Display anemometer server address";
		*comm_help.use = "dasa";

		*(comm_help.cmd+1) = "dara";
		*(comm_help.desc+1) = "Display anemometer register address";
		*(comm_help.use+1) = "dara";


		*(comm_help.cmd+2) = "darn";
		*(comm_help.desc+2) = "Display anemometer register number";
		*(comm_help.use+2) = "darn";

		*(comm_help.cmd+3) = "dtsa";
		*(comm_help.desc+3) = "Display Controller server address";
		*(comm_help.use+3) = "dtsa";

		*(comm_help.cmd+4) = "dtsp";
		*(comm_help.desc+4) = "Display Controller server port";
		*(comm_help.use+4) = "dtsp";

		*(comm_help.cmd+5) = "dtsr";
		*(comm_help.desc+5) = "Display Controller stow register address";
		*(comm_help.use+5) = "dtsr";

		*(comm_help.cmd+6) = "dtsn";
		*(comm_help.desc+6) = "Display Controller stow register number";
		*(comm_help.use+6) = "dtsn";

		*(comm_help.cmd+7) = "dstatr";
		*(comm_help.desc+7) = "Display Controller stow status register address";
		*(comm_help.use+7) = "dstatr";

		*(comm_help.cmd+8) = "sasa";
		*(comm_help.desc+8) = "Set anemometer server address";
		*(comm_help.use+8) = "sasa [X.X.X.X]";

		*(comm_help.cmd+9) = "sara";
		*(comm_help.desc+9) =  "Set anemometer register address";
		*(comm_help.use+9) = "sara [value]";

		*(comm_help.cmd+10) = "sarn";
		*(comm_help.desc+10) = "Set anemometer register number";
		*(comm_help.use+10) = "sarn [value]";

		*(comm_help.cmd+11) = "stsa";
		*(comm_help.desc+11) = "Set Controller server address";
		*(comm_help.use+11) = "stsa [X.X.X.X]";

		*(comm_help.cmd+12) = "stsp";
		*(comm_help.desc+12) = "Set Controller server port";
		*(comm_help.use+12) = "stsp [value]";

		*(comm_help.cmd+13) = "stsr";
		*(comm_help.desc+13) = "Set Controller stow register address";
		*(comm_help.use+13) = "stsr [value]";

		*(comm_help.cmd+14) = "stsn";
		*(comm_help.desc+14) = "Set Controller stow register number";
		*(comm_help.use+14) = "stsn [value]";

		*(comm_help.cmd+15) = "sstatr";
		*(comm_help.desc+15) = "Set Controller stow status register address";
		*(comm_help.use+15) = "sstatr [value]";

		//net help menu data
		*sys_help.cmd = "dra";
		*sys_help.desc = "Display Robustow IP address";
		*sys_help.use = "dria";

		*(sys_help.cmd+1) = "dtn";
		*(sys_help.desc+1) = "Display Controller number";
		*(sys_help.use+1) = "dtn";

		*(sys_help.cmd+2) = "dtas";
		*(sys_help.desc+2) = "Display Controller IP address(es)";
		*(sys_help.use+2) = "dtas";

		*(sys_help.cmd+3) = "sra";
		*(sys_help.desc+3) = "Set Robustow network interface";
		*(sys_help.use+3) = "sra [dhcp|static] [IP address] [Netmask] [Gateway]";

		*(sys_help.cmd+4) = "ripa";
		*(sys_help.desc+4) = "Reload Controller IP address(es) (Execute after uploading new address file)";
		*(sys_help.use+4) = "ripa [FILE NAME] (Case Sensitive)";

		*(sys_help.cmd+5) = "stz";
		*(sys_help.desc+5) = "Set Time Zone";
		*(sys_help.use+5) = "stz";
}
