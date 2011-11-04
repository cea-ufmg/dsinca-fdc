#ifndef _MESSAGES_H
#define _MESSAGES_H

// Comprimento maximo de strings.
#define MAX_STRLEN 64

// FIFO de controle do FDC
#define CTRL_FIFO "/tmp/fdc_ctrl"

// Biblioteca na qual esta incluida a time.h
#include <rtai_sched.h>

// Possiveis comandos para o FDC.
typedef enum {
    NO_CMD,
    START,
    STOP,
    QUIT,
    RESET_GPS,
    CHANGE,
    CHANGEDATFILE,  // Composicao de change + datfile
    CHANGETS,       // Composicao de change + ts
     NODATA,
    ENABLEDAQ,
    DISABLEDAQ,
    ASSIGN,
    FILTER_ON,
    FILTER_OFF,
    IS_ALIVE    // Serve para saber se o modulo de tempo real esta vivo
} fdc_cmd_t;

// Possiveis opcoes para os comandos.
typedef enum {
    NO_OPTION,
    TS,
    DATFILE,
    DAQCHANNEL,
    DAQ,
    GPS,
    AHRS,
    NAV,
    PITOT,
    MODEM,
    INTERNAL_TEMPERATURE,
    ENGINE_TEMPERATURE,
    ALPHA,
    BETA,
    PSTAT,
    PDYN,
    LOADCELL,
    ENGINE_RPM
} fdc_cmd_option_t;

// Valores de retorno para comandos enviados pelo 'fdc_master' para 'fdc_slave'
typedef enum {
       OK,
       NOT_OK,
       TIMEOUT
} cmd_status_t;

// Definicoes dos comandos enviados via modem para o fdc
enum {
       START_CMD,
       STOP_CMD,
    RESET_GPS_CMD
};

// Estrutura que acomoda diferentes tipos de variaveis
// no mesmo espaco de memoria.
    //typedef union {
    //    int n;
    //    float t;
    //} fdc_cmd_data_t;
// Por enquanto, basta um inteiro.
typedef int fdc_cmd_data_t;

// Estrutura da mensagem de comando a ser enviada
// do 'fdc_master' para 'fdc_slave'.
typedef struct {
   fdc_cmd_t cmd;
   fdc_cmd_option_t option;
   fdc_cmd_data_t data;
} cmd_msg_t;

// Estrutura de mensagem de comando a ser enviada
// do 'fdc_cmd_parser' para o 'fdc_master'.
// Obs.: Foi necessario adicionar uma string para conter o nome do arquivo
// de dados. Ex.: "change datfile gps12022005.dat".
typedef struct {
   cmd_msg_t msg;
   char name[MAX_STRLEN];
}parser_cmd_msg_t;

// Enumeracoes de dados das coordenadas da IMU
enum XYZIndices
{
    X_AXIS,    //!< X axis
    Y_AXIS,    //!< Y axis
    Z_AXIS,    //!< Z axis
    N3D          //!< Number of XYZ indices
};

////////////////////////////////////////////////////////////////////////////////////////////
/// DEFINICOES DAS MENSAGENS QUE SAO VISTAS PELO PROGRAMA DE MODO COMUM E TEMPO REAL
/// Diferem das mensagens do programa modulo de tempo real apenas pelo fato da variavel RTIME  
/// ser substituida por um tipo long long
/// RTIME = long long
////////////////////////////////////////////////////////////////////////////////////////////

/// DEFINICAO DO TIPO DE MENSAGEM A SER ENVIADA PELA FIFO DAQ    ////////////////////////////
typedef struct
    {
        int         validade;
        float         tensao[16];
        long long     time_sys;
    }  msg_daq_t;

/// DEFINICAO DO TIPO DE MENSAGEM A SER ENVIADA PELA FIFO IMU    ////////////////////////////
typedef struct
    {
        int validade;
        float gyro[3];
        float accel[3];
        double time_stamp;    // Tempo dado pela imu
        long long time_sys;    // Tempo do sistema
    }  msg_imu_t;

/// DEFINICAO DO TIPO DE MENSAGEM A SER ENVIADA PELA FIFO AHRS    ////////////////////////////
typedef struct
    {
        int validade;
        float angle[3];
        float gyro[3];
        float accel[3];
        float magnet[3];
        float time_stamp;    // Tempo dado pelo ahrs (zera a cada 50 ms)
        float temp; // temperatura interna do ahrs
        long long time_sys;    // Tempo do sistema
    }  msg_ahrs_t;

/// DEFINICAO DO TIPO DE MENSAGEM A SER ENVIADA PELA FIFO NAV    ////////////////////////////
typedef struct
    {
        int validade;
        float angle[3];
        float gyro[3];
        float accel[3];
        float nVel, eVel, dVel;
        float latitude, longitude, altitude;
        float temp;            // Temperatura interna do sensor
        int internal_error;
        int internal_status;
        long time_stamp;
        long long time_sys;    // Tempo do sistema
    }  msg_nav_t;

/// DEFINICAO DO TIPO DE MENSAGEM A SER ENVIADA PELA FIFO PITOT    ////////////////////////////
typedef struct
    {
        int validade;
        float static_pressure;
        float temperature;
        float dynamic_pressure;
        float attack_angle;
        float sideslip_angle;
        long long time_sys;    // Tempo do sistema
    }  msg_pitot_t;

/// DEFINICAO DO TIPO DE MENSAGEM A SER ENVIADA PELA FIFO GPS    ////////////////////////////

typedef struct 
    {
        //gga message attributes
        float latitude,longitude; //ddmm.mmmmm
        float altitude;
        float hdop;
        float geoid_separation;
           int north_south;        //north = 78, south = 83
           int east_west;            //west = 87, east = 69
           int fix_indicator;        // 0 = fix nao disponivel ou invalido
           int n_satellites;        // 0 a 12
           int units_altitude;        // meters = 77
           int units_geoid_separation;    //meters = 77
           float GPS_time_gga;        // retorna tempo em segundos desde o começo do dia
        //rmc message attributes
        float GPS_time_rmc;     // retorna tempo em segundo deste o começo do dia
        int status;             //A(valido) = 65 V(invalido) = 86
        float gspeed;           //Ground speed em kts
        float course;           //Course em graus
        int date;               // data ddmmyy
        float magvar;           //declinacao magnetica
        int magvardir;          // west = 87, east = 69
        int mode;               //
        //pgrmv message attributs
        float east_v, north_v, up_v;
        //pgrme message attributs
        float hpe, vpe, epe;
        int hpe_units, vpe_units, epe_units;
        //other stuff
        int validity;            // 1 = success, 0 = falha geral, 2 = falha timeout.
           long long time_sys;
    } msg_gps_t;

/// ESTRUTURA DE DADOS DA MSG DO MODEM ////////////////////////////////////////////////////////
typedef struct
    {
    // Time-stamp do pacote
        long long time_sys;    // Tempo do sistema no instante do envio
    
    //Dados da placa DAQ
        float tensao[16];

    //Dados da IMU
        float gyro[3];
        float accel[3];
        double time_stamp;
        
    //Dados do GPS
        float latitude,longitude,altitude,hdop,geoid_separation;
           int north_south;    //north = 78, south = 83
           int east_west;        //west = 87, east = 69
        int n_satellites;    // 0 a 12
    
    // Contador de pacotes
        unsigned char count_packets;
 }  msg_modem_t;
 
 /// Definiçoes do processo de modo usuario
 
 // Definicao das mensagens de log
#define STATUS_LOG     0     // messagem de status
#define ERROR_LOG    1    // mensagem de erro

// Possiveis estados do programa fdc_master
typedef enum {
    RUNNING, // Caso apos o comando START. A thread que salva dados esta em execucao
    STOPPED,     // Caso apos o comando STOP. A thread eh terminada
    QUITTED     // Caso apos comando QUIT. O programa eh terminado
} fdc_state_t;

#endif

