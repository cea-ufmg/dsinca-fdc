/*!*******************************************************************************************
*********************************************************************************************/
///				BIBLIOTECA DA IMU
/*!*******************************************************************************************
*********************************************************************************************/
#ifndef _IMU_SERIAL_H
#define _IMU_SERIAL_H

#include "rtai_crc.h"
#include "rtai_rt_serial.h"
#include "messages.h"

//Define serial sync characters
#define SYNC_IMU_SER_0 0x55
#define SYNC_IMU_SER_1 0xAA

//Define message header length
#define MESSAGE_HEADER_LEN 4 //2 sync characters, type, length
//Define maximum message length in bytes, including the header
#define MAX_MESSAGE_LEN (MESSAGE_HEADER_LEN+18+2)

//!Conversion constants (method and data subject to change)
//Constants to convert from deg/s or m/s^2 to resolution units.
// Multiply deg/s or m/s^2 to get resolution units.
// Divide resolution units by CONV_GYRO_RESOLUTION or CONV_ACCEL_RESOLUTION to get deg/s or m/s^2.
// The size of the resolution unit is 1/CONV_GYRO_RESOLUTION
// This converts from gyro output in deg/sec to gyro units, for encoding in an signed short.
// If you divide raw gyro resolution counts by this number, you get deg/sec.
//Units: gyro resolution units / (deg/s)
#define CONV_GYRO_RESOLUTION  (65536.0f/(300.0f*2)) //109.22666666667f

//If you divide raw accelerometer resolution counts by this number, you get m/s^2.
//Units: accel resolution units / (m/s^2)
#define CONV_ACCEL_RESOLUTION (65536.0f/(10.0f*2.0f*9.805f)) //334.1968383478f

//! Timer frequency (may change in future)
#define TIMER_FREQUENCY_HZ (8000000UL)

//!Conversion for raw data
#define COUNTS_TO_VOLTS_AD16 (4.096/65536.0)

// Configuracoes da IMU para taxa de amostragem interna (oversample x samplerate)
#define IMU_OVERSAMPLE 		20 
#define IMU_SAMPLERATE_HZ  	50.0f

// Define o periodo de execucao da tarefa da imu para 20 ms (ou 50 Hz)
#define IMU_PERIOD	20	

// Tempo padrao de periodo de amostragem
#define UM_MILI_SEGUNDO 1000000

// Prioridade default da tarefa de tempo real
#define IMU_TASK_PRIORITY 1

// Definicoes do Filtro Butterworth de 4 ordem com frequencia de corte de 15 Hz em 25 Hz de amostragem
#define A0	0.1672
#define A1	0.6687
#define A2	1.0031
#define A3	0.6687
#define A4	0.1672

//#define B0	1 	//eh sempre igual a um 
#define B1	0.7821
#define B2	0.6800
#define B3	0.1827
#define B4	0.0301

// Definicao da ordem do filtro
#define FILTER_ORDER	4

//O indice "i" do vetor corresponde ao tempo k atrasado de i -> x(k-i) = x[i].
#define K	0
#define K_1	1	// Tempo discreto (k-1)
#define K_2	2
#define K_3	3
#define K_4	4

//! Enumeracao das messages trocadas entre o host e a IMU
//! IMU CAN/Serial Message type enumerations
enum IMUMessageTypes
{
	RAWGYRO_IMU_MSG,		//!< Raw gyro data
	RAWGYROTEMP_IMU_MSG,		//!< Raw gyro temperatures
	RAWACCEL_IMU_MSG,		//!< Raw accelerometer data
	TIMING_IMU_MSG,			//!< Timing data
	RESOLUTION_IMU_MSG,		//!< Resolution
	RESUNITS_GYRO_IMU_MSG,		//!< Gyro data in resolution units
	RESUNITS_ACCEL_IMU_MSG,		//!< Accelerometer data in resolution units
	SET_SETTINGS_IMU_MSG,		//!< Set settings IMU message
	SETTINGS_IMU_MSG,		//!< Return settings IMU message
	MFRCALDATE_IMU_MSG,		//!< Return manufacture and calibration dates
	SERIALNUMCONFIG_IMU_MSG,	//!< Return serial number and configuration data
	SWVERSION_IMU_MSG,		//!< Return software versions message
	BOARDREFERENCE_IMU_MSG,		//!< Return processor board reference data (crystal frequency) 
	REQ_CONFIG_IMU_MSG,		//!< Request all configuration and settings messages

	//Add new messages before this line.
	HS_SERIAL_IMU_MSG = 0xFF   	//!< High speed serial packet - This is never sent as a
					//!<   CAN packet, just as a packaged serial packet.
};
	
//!Enumerate communications modes
enum CommunicationsModes {
	RS232_COMMS_MODE=1,
	CAN_COMMS_MODE,
	BOTH_COMMS_MODE
};

//!Enumerate sample modes
enum SampleModes {
	CONVERTED_SAMPLE_MODE=1,
	RAW_SAMPLE_MODE,
	BOTH_SAMPLE_MODE
};

// Variavel global da tarefa de tempo real da imu
RT_TASK task_imu;

// variavel global de dados da imu
msg_imu_t global_msg_imu;

// Variavel global para avisar ao padawan sobre um dado novo
int global_new_data = 0;

/*!*******************************************************************************************
*********************************************************************************************/
///				FUNCOES DA IMU
/*!*******************************************************************************************
*********************************************************************************************/
//!Process the serial state machine to receive IMU packets.
//! This routine can be called multiple times to accumulate a packet.
int rt_process_imu_serial(unsigned char* MessageBuffer, int *type_message);

///////////////////////////////////////////////////////////////////////////////////////////////////////
int rt_convert_imu_data(msg_imu_t* msg, unsigned char msgbuf[]);

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Esta funcao transmite qualquer pacote de dados atraves da porta serial da IMU
	- MessageType corresponde ao tipo de mensagem enviada
	- pMessageBuffer contem a mensagem a ser enviada
	- len eh o tamanho da mensagem enviada*/
void rt_transmit_serial_message(int fd, unsigned char MessageType, unsigned char* pMessageBody,unsigned char len);

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Envia mensagem que seta os parametros da IMU.
	- SampleRateHz eh a taxa de amostragem em Hz
	- OversampleRatio eh a taxa de superamostragem - Geralmente vale 255 
	- CommsMode eh o modo de comunicacao (neste caso serial)
	- SampleMode eh a forma como os dados sao amostrados (convertidos ou crus)*/
void rt_send_imu_settings_serial_message(float SampleRateHz,unsigned short OversampleRatio,unsigned char CommsMode, unsigned char SampleMode);

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Envia mensagem requisitando a configuracao da IMU*/
void rt_request_config_imu_serial_message(void);

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Implementa um filtro digital Butterwoth de 4 ordem com frequencia de corte de 300 Hz*/
float filter_data(float x, float m[]);
//////////////////////////////////////////////////////////////////////////////////////////////////////
int rt_filter_imu_data(msg_imu_t* msg);

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Abre a imu para comunicacao e configura a frequencia de comunicacao desejada*/
int rt_open_imu(void);

//////////////////////////////////////////////////////////////////////////////////////////////////////
int rt_get_imu_data(msg_imu_t *msg);

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Funcao executada pela tarefa de tempo real da imu*/
void func_imu(int t);

#endif

