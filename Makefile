# Projeto DSINCA/FAPEMIG - UFMG - 10/2008

FLEX = flex
CC = gcc

SRCDIR = ./src
INCLUDEDIR = ./include
OBJDIR = ./object

CFLAGS = -Wall -O2 -I$(INCLUDEDIR)
MFLAGS = -D__KERNEL__ -DMODULE -O2 -Wall -I$(INCLUDEDIR)

RT_KERNEL = /usr/src/linux
RTAI = /usr/realtime
INCLUDE = -I$(RT_KERNEL)/include -I$(RTAI)/include

# Indica diretorios onde devem ser procurados os arquivos
VPATH = ./src:./include:./object

########################################################################################
all : save_data ioSockets fdc_master fdc_cmd_parser rtai_gps rtai_daq rtai_ahrs rtai_nav rtai_pitot fdc_slave

## Thread de salvamento dos dados
save_data : save_data.c save_data.h messages.h fdc_structs.h
	##
	## save_data
	##
	g++ $(CFLAGS) $(INCLUDE) -c $< -o $(OBJDIR)/$@.o
	##

## Funcao de envio dos dados
ioSockets : ioSockets.cpp ioSockets.h
	##
	## ioSocket
	##
	g++ $(CFLAGS) $(INCLUDE) -c $< -o $(OBJDIR)/$@.o
	##

## Processo principal que cuida da comunicacao com a
## tarefa de tempo real
fdc_master : fdc_master.c fdc_master.h messages.h fdc_structs.h save_data.h save_data.c ioSockets.cpp paralela_leds.h
	##
	## fdc_master
	##
	g++ $(CFLAGS) $(INCLUDE) ./object/ioSockets.o ./object/save_data.o $(SRCDIR)/acel.cpp $(SRCDIR)/giro.cpp $(SRCDIR)/quaternio.cpp $(SRCDIR)/euler.cpp -lpthread $< -o $@
	##

## Sub-processo responsavel pela analise lexicografica de comandos
## enviados na FIFO de controle do FDC, que repassa as mensagens
## jah processadas para fdc_master.
fdc_cmd_parser : fdc_cmd_parser.c fdc_cmd_parser.h messages.h
	$(CC) $(CFLAGS) $(INCLUDE) ./src/fdc_cmd_parser.c -o $@
		
## Codigo C do analisador lexicografico gerado automaticamente
## pelo pacote "flex".
fdc_cmd_parser.c : fdc_cmd_parser.y
	##
	## fdc_cmd_parser
	##
	$(FLEX) -o$(SRCDIR)/fdc_cmd_parser.c $<
	##

## Modulo do kernel responsavel por coletar os dados do gps no pc104
## Autor: Flavio Mota
rtai_gps: rtai_gps.c rtai_gps.h
	##
	## rtai_gps
	##	
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $(OBJDIR)/$@.o
	##

## Funcoes de acesso a placa daq
rtai_daq: rtai_daq.c rtai_daq.h
	##
	## rtai_daq
	##	
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $(OBJDIR)/$@.o
	##

## Funcoes de acesso ao ahrs
rtai_ahrs: rtai_ahrs.c rtai_ahrs.h
	##
	## rtai_ahrs
	##	
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $(OBJDIR)/$@.o
	##

## Funcoes de acesso ao nav
rtai_nav: rtai_nav.c rtai_nav.h
	##
	## rtai_nav
	##	
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $(OBJDIR)/$@.o
	##

## Funcoes de acesso ao pitot
rtai_pitot: rtai_pitot.c rtai_pitot.h
	##
	## rtai_pitot
	##	
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $(OBJDIR)/$@.o
	##

## Modulo de tempo real para a captura dos dados no uav. 
## Estes dados sao enviados para o programa uav_jedi e para a estacao de solo
fdc_slave: fdc_slave.c fdc_slave.h messages.h rtai_crc.h rtai_rt_serial.h rtai_daq.h rtai_ahrs.h rtai_gps.h rtai_nav.h rtai_picopic.h rtai_modem.h
	##
	## fdc_slave
	##
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $(OBJDIR)/$@.o
	##
	
########################################################################################	
## Instalacao e retirada dos modulos
in:
	insmod $(RTAI)/modules/rtai_hal.o
	insmod $(RTAI)/modules/rtai_ksched.o
	insmod $(RTAI)/modules/rtai_sem.o
	insmod $(RTAI)/modules/rtai_serial.o
	insmod $(RTAI)/modules/rtai_fifos.o
	insmod rtai_daq.o
	insmod rtai_ahrs.o
	insmod rtai_gps.o
	insmod rtai_nav.o
	insmod rtai_pitot.o
	insmod fdc_slave.o

rm:
	rmmod fdc_slave
	rmmod rtai_pitot
	rmmod rtai_nav
	rmmod rtai_gps
	rmmod rtai_ahrs
	rmmod rtai_daq
	rmmod rtai_fifos
	rmmod rtai_serial
	rmmod rtai_sem
	rmmod rtai_ksched
	rmmod rtai_hal
	
########################################################################################	

.PHONY : clean
clean :
	@rm -f ./include/*~ *~ ./src/*~ *.bak *.o $(OBJDIR)/* ./src/fdc_cmd_parser.c fdc_master fdc_cmd_parser

.PHONY : backup
backup : clean
	tar czvf ../fdc_code.tgz ./*


