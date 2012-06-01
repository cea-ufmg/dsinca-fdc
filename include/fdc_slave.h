#ifndef _FDC_SLAVE
#define _FDC_SLAVE

#include <rtai_fifos.h>

#include "rtai_daq.h"        /* Biblioteca da placa DAQ  */
#include "rtai_ahrs.h"       /* Biblioteca da AHRS       */
#include "rtai_gps.h"        /* Biblioteca do GPS        */
#include "rtai_nav.h"        /* Biblioteca do NAV        */
#include "rtai_pitot.h"      /* Biblioteca do PITOT      */

// Mensagens de comunicacao
#include "messages.h"

// Define a utilizacao de ponto flutuante dentro do kernel
#ifndef CONFIG_RTAI_FPU_SUPPORT
    #define CONFIG_RTAI_FPU_SUPPORT
#endif

// Tempo padrao de periodo de amostragem
#define UM_MILI_SEGUNDO 1000000

// Multiplicador do periodo da tarefa usado foram de nano2count() - valor em milisegundos
// (1 ms*20)^-1 = 50 Hz (empiricamente não pode ser maior do que 2 segundos)
#define PERIOD        20        

// Prioridade default da tarefa de tempo real
#define TASK_PRIORITY 1

//    DEFINICAO DAS FIFOS DE TEMPO REAL    //////////////////////////////////////////////
#define RT_FIFO_AHRS     0
#define RT_FIFO_DAQ     1
#define RT_FIFO_GPS     2
#define RT_FIFO_NAV        3
#define RT_FIFO_PITOT    4
#define RT_FIFO_CONTROL 5
#define RT_FIFO_STATUS     6
#define RT_FIFO_COMAND     7    // Comandos recebidos via modem

// Numero maximo de pacotes perdidos na comunicacao via modem
#define MAX_PACKETS_LOST 100    

// Frequencia de execucao da tarefa de tempo real
#define FREQUENCY 50

// Taxas de execucao de comandos em tempo real
#define _01_HZ     FREQUENCY
#define _05_HZ     FREQUENCY/5
#define _50_HZ     FREQUENCY/50

/*!*******************************************************************************************
*********************************************************************************************/
///                VARIAVEIS GLOBAIS
/*!*******************************************************************************************
*********************************************************************************************/

/// Estrutura de  configuracao do modulo
typedef struct {  
  // Indicadores que servem para habilitar individualmente os dispositivos ou inicializar todos    
  int daq_enable;
  int ahrs_enable;
  int gps_enable;
  int modem_enable;
  int nav_enable;
  int pitot_enable;
  int servo_enable;
}  configure;

static void rt_func_daq(configure* config);

static void rt_func_ahrs(configure* config);

static void rt_func_gps(configure* config);

static void rt_func_nav(configure* config);

static void rt_func_pitot(configure* config);

//static void rt_func_modem(configure* config);

static int  rt_func_control(configure* config);

static void func_fdc_slave(int t);

static int terminate_module(void);

int init_module(void);
int cleanup_module(void);

#endif
