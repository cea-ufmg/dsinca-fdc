/*
	rtai_ahrs.h - AHRS400CD-200 module header
    Copyright (C) 2010  Víctor Costa da Silva Campos - kozttah@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RTAI_AHRS_H
#define RTAI_AHRS_H

#include "rtai_rt_serial.h"
#include "messages.h"

/*--------------------------------------------------------------------------------------------
					AHRS COMANDS AND RESPONSES
--------------------------------------------------------------------------------------------*/

//Communication Testing Messages
//Ping
#define PING  0x52 //R
#define PING_RESPONSE 0x48 //H

//Measurement Mode Configuration Messages
//Voltage Mode
#define VOLTAGE_MODE 0x72 //r
#define VOLTAGE_MODE_RESPONSE 0x52 //R
//Scaled Mode
#define SCALED_MODE 0x63//c
#define SCALED_MODE_RESPONSE 0x43 //C
//Angle Mode
#define ANGLE_MODE 0x61 //a
#define ANGLE_MODE_RESPONSE 0x41 //A

//Communication Mode Configuration Messages
/*
	For this application we'll be using the angle (vg) mode exclusively.
	Therefore the data packets received we'll be of a single structure.
*/
//Polled Mode
#define POLLED_MODE 0x50 //P - there is no response message
//Continuous Mode
#define CONTINUOUS_MODE 0x43 //C - the AHRS will start outputing data packets continuously
//Request Data - used in Polled Mode
#define REQUEST_DATA 0x47 //G - will output one data packet

//Querying AHRS information Messages
//Version
#define QUERY_VERSION 0x76 //v - will return an ASCII string describing AHRS type and firmware version
#define QUERY_VERSION_LENGTH 26 //verified it with hyperterminal (header and checksum included)
//Serial Number
#define QUERY_SERIAL_NUMBER 0x53 //S - will return a serial number packet
/* 
	Serial Number Packet:
	0xFF - Header byte
	SN_body - four bytes unsigned int (uint_32)
	chk - checksum byte
*/

//Autodetect baud rate Messages
/*
	Autodetect baud rate procedure:
	Using the precofigured baud rate:
		Send a REQUEST_BAUD message and wait for the REQUEST_BAUD_RESPONSE
	Using the new (desired) baud rate:
		Send a NEW_BAUD message and wait for the NEW_BAUD_RESPONSE at the new baud rate
*/
#define REQUEST_BAUD 0x62 //b
#define REQUEST_BAUD_RESPONSE 0x42 //B
#define NEW_BAUD 0x61 //a
#define NEW_BAUD_RESPONSE 0x41 //A

//Magnetic Calibration Messages
//Start Hard/Soft iron calibration
#define START_CALIB 0x73//s
#define START_CALIB_RESPONSE 0x53//S
//End Hard/Soft iron calibration
#define END_CALIB 0x75//u
#define END_CALIB_RESPONSE 0x55//U
//Clear Hard iron calibration
#define CLEAR_HARD 0x68//h
#define CLEAR_HARD_RESPONSE 0x48//H
//Clear Soft iron calibration
#define CLEAR_SOFT 0x74//t
#define CLEAR_SOFT_RESPONSE 0x54//T

//Define AHRS message's constants
#define AHRS_HEADER 0xFF
#define AHRS_MSG_LEN 29 //header not included (crc included)

/*--------------------------------------------------------------------------------------------
					AHRS DATA CONVERSION
--------------------------------------------------------------------------------------------*/

//Conversion constants
#define RATE_RANGE 200
#define G_RANGE 4

//Conversion macros
//Internal Temperature Conversion - input uint_16 / output double in ºC
#define AHRS_RAW2TEMP(Vraw) (44.44*((Vraw*5.0)/(4096.0) - 1.375))

//Timer Conversion - input uint_16 / output double in microseconds (counts from 0 to 50ms only)
#define AHRS_RAW2TIME(Traw) ((65535 - Traw)*0.79)

//Angle Conversion - input int_16 / output double in º
#define AHRS_RAW2ANGLE(Araw) (Araw*180.0/32768.0)

//Magnetic Field Conversion - input int_16 / output double in Gauss
#define AHRS_RAW2MAG(Mraw) (Mraw*1.25*1.5/32768.0)

//Rate Conversion - input int_16 / output double in º/s
#define AHRS_RAW2RATE(Rraw) (Rraw*RATE_RANGE*1.5/32768.0)

//Acceleration Conversion - input int_16 / output double in g
#define AHRS_RAW2ACCEL(Araw) (Araw*G_RANGE*1.5/32768.0)

/*--------------------------------------------------------------------------------------------
					AHRS COMMUNICATION DEFAULTS

	RS-232
	baud rate - 38400 (default)
	8 data bits
	1 start bit
	1 stop bit
	no parity
	no handshake (flow control)
--------------------------------------------------------------------------------------------*/
#define AHRS_DEFAULT_BAUD 38400

/*--------------------------------------------------------------------------------------------
					AHRS CONSTANTS AND GLOBAL VARIABLES
--------------------------------------------------------------------------------------------*/

// Defines the ahrs task period to 20 ms (50 Hz)
#define AHRS_PERIOD	20	

// Standard sampling period time
#define A_MILLI_SECOND 1000000

// Real Time task default priority
#define AHRS_TASK_PRIORITY 1

// AHRS Real Time Task Global Variable
RT_TASK task_ahrs;

// AHRS data Global Variable
msg_ahrs_t global_msg_ahrs;

// Variavel global para avisar ao slave sobre um dado novo
int global_new_data = 0;

/*--------------------------------------------------------------------------------------------
					AHRS FUNCTIONS
--------------------------------------------------------------------------------------------*/
// Opens the AHRS communication
int rt_open_ahrs(void);

//Configures the AHRS communication
int rt_cfg_ahrs(void);

// Main function executed by the AHRS real time task
void func_ahrs(int t);

//Convert the message received (msgbuf) to engineering units (msg)
int rt_convert_ahrs_data(msg_ahrs_t* msg,unsigned char* msgbuf);

//Gets a data packet
int rt_process_ahrs_serial(unsigned char* MessageBuffer);

//checks the message checksum and returns 1 (correct) or 0 (wrong)
//checksum is available on the last msg byte
int rt_chksum_check(unsigned char* MessageBuffer);

//Allows the other module (fdc_slave) to get the ahrs data
//The function returns 1 for new data and 0 for old data
int rt_get_ahrs_data(msg_ahrs_t *msg);

//Transform a 2-complement word (2 bytes) to a common int
int convert_int_data(unsigned char msb,unsigned char lsb);

#endif

