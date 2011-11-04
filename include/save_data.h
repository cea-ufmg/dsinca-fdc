/*!*******************************************************************************************
**********************************************************************************************
            BIBLIOTECA PARA SALVAMENTO DOS DADOS
*********************************************************************************************
********************************************************************************************/
#ifndef _SAVE_DATA
#define _SAVE_DATA

#include "fdc_structs.h"
//#include "ioSockets.h"


// Nomes de arquivos default
#define ARQ_DAQ        "daq_file.dat"
#define ARQ_AHRS    "ahrs_file.dat"
#define ARQ_GPS        "gps_file.dat"
#define ARQ_NAV        "nav_file.dat"
#define ARQ_PITOT    "pitot_file.dat"

// Diretoria que abriga os arquivos
#define FILES_PATH "/tmp/data/"

// Valor de conversao da tensao nos canais da placa daq
#define CONV_DATA_CHANNEL (5.0f/4095.0f)

// Define a variavel global do programa fdc_jedi
extern global_master global;

extern int master_log(int type_message, const char* place);

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para escrita dos cabecalhos dos arquivos
int write_headers (FILE* arq_daq, FILE* arq_imu, FILE* arq_gps, FILE* arq_nav, FILE* arq_pitot);

/*!*******************************************************************************************
 * *********************************************************************************************/
// Funcao para a leitura dos dados da fifo da placa daq
int get_daq();

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para armazenagem dos dados da placa DAQ no  arquivo da placa daq
int save_daq(FILE* arquivo_daq);

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para a leitura dos dados da fifo do ahrs imu e armazenagem destes dados no 
// arquivo da imu
int get_ahrs();
int save_ahrs(FILE* arquivo_ahrs);

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para a leitura dos dados da fifo do nav e armazenagem destes dados no 
// arquivo da nav
int get_nav();
int save_nav(FILE* arquivo_nav);

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para a leitura dos dados da fifo do pitot e armazenagem destes dados no 
// arquivo do pitot
int get_pitot();
int save_pitot(FILE* arquivo_pitot);


/*!*******************************************************************************************
 * *********************************************************************************************/
// Funcao para a leitura dos dados da fifo do gps
int get_gps();

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para armazenagem dos dados do GPS dados no  arquivo do gps
int save_gps(FILE* arquivo_gps);

/*!*******************************************************************************************
*********************************************************************************************/
int create_new_dir (void);

/*!*******************************************************************************************
*********************************************************************************************/
/*    Thread para a coleta dos dados. Primeiramente eh resetada a variavel de fim da thread,
 e na sequencia sao abertos os arquivos default ou novos.Entao se escreve os cabecalhos dos
 arquivos e so entao passasse a captar os dados, ate que a thread seja terminada.*/
void *save_data(void *arg);

#endif
