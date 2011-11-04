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

################################################################################
all: fdc_master fdc_cmd_parser object/rtai_gps.o object/rtai_daq.o \
     object/rtai_ahrs.o object/rtai_nav.o object/rtai_pitot.o object/fdc_slave.o\
     object/epos.o object/epos_debug.o

## Thread de salvamento dos dados
object/save_data.o : src/save_data.c include/save_data.h include/messages.h include/fdc_structs.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@


## Processo principal que cuida da comunicacao com a
## tarefa de tempo real
fdc_master: src/fdc_master.c object/save_data.o fdc_master.h messages.h fdc_structs.h save_data.h
	$(CC) $(CFLAGS) $(INCLUDE) ./object/save_data.o -lpthread $< -o $@

## Sub-processo responsavel pela analise lexicografica de comandos
## enviados na FIFO de controle do FDC, que repassa as mensagens
## jah processadas para fdc_master.
fdc_cmd_parser: src/fdc_cmd_parser.c include/fdc_cmd_parser.h include/messages.h
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

## Codigo C do analisador lexicografico gerado automaticamente
## pelo pacote "flex".
src/fdc_cmd_parser.c: src/fdc_cmd_parser.y
	$(FLEX) -o$@ $<

## Modulo de tempo real para a captura dos dados no uav. 
## Estes dados sao enviados para o programa uav_jedi e para a estacao de solo
object/fdc_slave.o: src/fdc_slave.c include/fdc_slave.h include/messages.h include/rtai_rt_serial.h include/rtai_daq.h include/rtai_ahrs.h include/rtai_gps.h include/rtai_nav.h
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDEDIR)/%.h
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/epos_debug.o: $(SRCDIR)/epos_debug.c
	$(CC) $(MFLAGS) $(INCLUDE) -c $< -o $@

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


