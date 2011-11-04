// Definicao das estruturas do sistema
#ifndef _FDC_STRUCTS_H
#define _FDC_STRUCTS_H

#include "messages.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

// Definições para controle da thread de salvamento de dados, que agora pode também enviar
// A variável global.end_save_data receberá um destes valores
#define STOPPED   0
#define SAVE      1
#define SEND      2
#define SAVE_SEND 3



typedef struct {
    // Variável global que indica o fim do programa UAV_PADAWAN
    int theend;

    // Variavel que indica o fim da thread de salvamento dos dados
    int end_save_data;
    
    // Nome do arquivo de configuracao.
    char config_file[MAX_STRLEN];
    
    // Nomes dos arquivos de salvamento de dados
    char file_daq_name[MAX_STRLEN], file_ahrs_name[MAX_STRLEN], file_gps_name[MAX_STRLEN], file_nav_name[MAX_STRLEN], file_pitot_name[MAX_STRLEN];

    // Descritor de arquivo da FIFO de controle
    FILE *ctrl_fifo;
    
    // Descritor de arquivo do log
    FILE *log_file;

    // Descritores (pontas) do pipe entre 'fdc_master' e 'fdc_cmd_parser'.
    int mypipe[2];

    // PID do processo 'fdc_cmd_parser'
    pid_t child;
    
    // Thread de salvamento de dados
    pthread_t salva_dados;
    
    //Fifos de leituras de dados dos dispositivos
    int fifo_daq, fifo_ahrs, fifo_gps, fifo_nav, fifo_pitot;
    
    // Fifos de status e control
    int fifo_status, fifo_control;
    
    // Fifo de comandos via modem
    int fifo_cmd;
    
    // Descricoes de semaforos para as variaveis globais
    // Nomes dos arquivos que armazenam os dados
    sem_t file_names;
    
    // Finalizar a thread de salvamento de dados
    sem_t end_thread_save_data;
    
    // Semaforo de acesso ao arquivo de log
    sem_t sem_log_file;
    
    // Estado em que o programa se encontra
    fdc_state_t state;
    
    // Estados dos LEDs de indicacao
    unsigned char led_byte;

} global_master;

#endif


