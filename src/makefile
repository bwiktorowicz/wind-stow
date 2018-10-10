all: stowcontroller

#cc main.o init_rs485.o wind_monitor.o set_ctr.o init_ctrl.o modbus_srv.o init_iface.o write_cfg.o net_cfg.o stw_ctrl.o forcst_dat.o set_ctrl_fault.o ctrl_util.o get_ctrl_adr.o -g -lncurses -pthread -lcurl -o stowcontroller -lm -L /usr/lib/libmodbus.* /usr/local/lib/libjsmn*

stowcontroller: main.o init_rs485.o wind_monitor.o set_ctr.o init_ctrl.o modbus_srv.o init_iface.o write_cfg.o net_cfg.o stw_ctrl.o forcst_dat.o set_ctrl_fault.o ctrl_util.o get_ctrl_adr.o
	cc main.o init_rs485.o wind_monitor.o set_ctr.o init_ctrl.o modbus_srv.o init_iface.o write_cfg.o net_cfg.o stw_ctrl.o forcst_dat.o set_ctrl_fault.o ctrl_util.o get_ctrl_adr.o -g -lncurses -pthread -lcurl -lmodbus -ljsmn -o stowcontroller -lm -L /usr/local/lib/libjsmn*

main.o: main.c
	cc -g -c main.c

init_rs485.o: init_rs485.c
	cc -g -c init_rs485.c

wind_monitor.o: wind_monitor.c
	cc -g -c wind_monitor.c

set_ctr.o: set_ctr.c
	cc -g -c set_ctr.c

#ctrl_stat.o: ctrl_stat.c
#	cc -g -c ctrl_stat.c

init_ctrl.o: init_ctrl.c
	cc -g -c init_ctrl.c

modbus_srv.o: modbus_srv.c
	cc -g -c modbus_srv.c

init_iface.o: init_iface.c
	cc -g -c init_iface.c

write_cfg.o: write_cfg.c
	cc -g -c write_cfg.c

net_cfg.o: net_cfg.c
	cc -g -c net_cfg.c

stw_ctrl.o: stw_ctrl.c
	cc -g -c stw_ctrl.c

forcst_dat.o: forcst_dat.c
	cc -g -c forcst_dat.c

set_ctrl_fault.o: set_ctrl_fault.c
	cc -g -c set_ctrl_fault.c

ctrl_util.o: ctrl_util.c
	cc -g -c ctrl_util.c

get_ctrl_adr.o: get_ctrl_adr.c
	cc -g -c get_ctrl_adr.c

clean:
	rm -rf stowcontroller core

distclean:
	rm -rf *.o stowcontroller core
