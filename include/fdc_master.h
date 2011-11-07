/*!*******************************************************************************************
**********************************************************************************************
            Arquivo Cabecalho FDC_MASTER
            Dispositivos incluidos: Placa DAQ
            DATA 06/01/2009
*********************************************************************************************
********************************************************************************************/
#ifndef _FDC_MASTER
#define _FDC_MASTER

// Definicao necessaria para se usar a funcao "strsignal"
#define _GNU_SOURCE

// Biblioteca de mensagens trocas pelos programas comum e de tempo real
#include "fdc_structs.h"

// DEFINICAO DAS FIFOS DE TEMPO REAL
#define FIFO_AHRS       "/dev/rtf0"
#define FIFO_DAQ        "/dev/rtf1"
#define FIFO_GPS        "/dev/rtf2"
#define FIFO_NAV        "/dev/rtf3"    
#define FIFO_PITOT      "/dev/rtf4"
#define FIFO_CONTROL    "/dev/rtf5"
#define FIFO_STATUS     "/dev/rtf6"
#define FIFO_COMMAND    "/dev/rtf7"

#define PARSER_NAME "fdc_cmd_parser"

#define DEFAULT_CONFIG_FILE "./fdc.conf"

#define LOG_FILE "./fdc.log"

#define PARSER_TIMEOUT 10

#define SEM_SIZE 1

// Define a variavel global do programa
global_master global;

void initialize(void);

void parse_args(int argc,char *argv[]);

pid_t fdc_cmd_parser(const char *args);

int load_config_file(void);

void main_loop(void);

void process_message(void);

void terminate(int s);

cmd_status_t sendcommand(parser_cmd_msg_t* parser_msg_to_rt);

int fdc_log(int type_message, const char* place);

int load_modules(void);

int unload_modules(void);

int modem_command(void);

// Esta bibliotecas devem ser declaradas no final
#include "save_data.h"

#endif
