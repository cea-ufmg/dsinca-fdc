/*
    rtai_nav.h - NAV440CA-400 module header
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

#ifndef RTAI_NAV_H
#define RTAI_NAV_H

#include "rtai_rt_serial.h"
#include "messages.h"

//Define NAV message's constants
//The header is composed of 0x5555 (UU) (repeat the NAV_HEADER_CHAR twice)
#define NAV_HEADER_CHAR 0x55 // U

/*--------------------------------------------------------------------------------------------
                    NAV COMANDS AND RESPONSES
--------------------------------------------------------------------------------------------*/

//Link Test
//Ping and ping response - Input/Reply - No CRC required when sending
#define PING1 0x50 //P
#define PING2 0x4B //K
//Echo command and echo command response - Input/Reply - Command + echo msg + CRC
#define ECHO1 0x43 //C
#define ECHO2 0x48 //H

/*--------------------------------------------------------------------------------------------
                    INTERACTIVE COMMANDS
--------------------------------------------------------------------------------------------*/
//Get Packet Request - Input - Command + packet type + CRC
#define GET_PACKET1 0x4750 //GP
//Packet types - also output msgs
#define NAV_PACK  0x4E //N
#define NUM1 0x31 //1
//Packet N1 should have 42 payload bytes

// -----------------------------------------------------> PAREI AQUI -> Página 40 do manual

//Algorithm Reset - Input/Reply
#define ALG_RESET 0x4152 //AR
//Software Reset - Input/Reply
#define SOFT_RESET 0x5352 //SR
//Error response - Reply
#define NAK 0x1515 //NAK
//Calibrate Command and response - Input/Reply
#define CALIBRATE 0x5743 //WC
//Calibration Completed - Reply
#define CALIBRATE_COMPLETE 0x4343 //CC


/*--------------------------------------------------------------------------------------------
                    NAV DATA CONVERSION
--------------------------------------------------------------------------------------------*/

//Conversion macros
//Internal Temperature Conversion - input int_16 / output double in ºC
#define NAV_RAW2TEMP(Traw) (200.0*Traw/65536.0)

//Timer Conversion - input uint_32 / output uint_32 in ms - no conversion needed
#define NAV_RAW2TIME(Traw) (Traw)

//Angle Conversion - input int_16 / output double in º
#define NAV_RAW2ANGLE(Araw) (Araw*360.0/65536.0)

//Rate Conversion - input int_16 / output double in º/s
#define NAV_RAW2RATE(Rraw) (Rraw*1260.0/65536.0)

//Acceleration Conversion - input int_16 / output double in g
#define NAV_RAW2ACCEL(Araw) (Araw*20.0/65536.0)

//Speed Conversion - input int_16 / output double in m/s
#define NAV_RAW2VEL(Vraw) (512.0*Vraw/65536.0)

//lat\long conversion - input int_32t / output double in º
#define NAV_RAW2LAT(Lraw) ((Lraw*360.0)/4294967296.0)

//Altitude conversion - input int_16t / output double in m
#define NAV_RAW2ALT(Araw) (Araw*16384.0/65536.0)

//Conversion functions
//Transform a 2-complement word (2 bytes) to a common int
int convert_int_data(unsigned char msb,unsigned char lsb);

//Transform a 2-complement double word (4 bytes) to a common int
int convert_int_data2(unsigned char msb1,unsigned char lsb1, unsigned char msb2, unsigned char lsb2);

/*--------------------------------------------------------------------------------------------
                    NAV STATUS FIELD MASKS
                    0 - means normal
                    1 - means error/status active
--------------------------------------------------------------------------------------------*/
#define MASTER_ERROR    0x01
#define HARD_ERROR        0x02
#define COM_ERROR        0x04
#define SOFT_ERROR        0x08
#define MASTER_STATUS    0x01
#define HARD_STATUS        0x02
#define COM_STATUS        0x04
#define SOFT_STATUS        0x08
#define SENSOR_STATUS    0x10

/*--------------------------------------------------------------------------------------------
                    NAV COMMUNICATION DEFAULTS

    RS-232
    baud rate - 38400 (default)
    8 data bits
    1 start bit
    1 stop bit
    no parity
    no handshake (flow control)
--------------------------------------------------------------------------------------------*/
#define NAV_DEFAULT_BAUD 57600
#define NAV_MSG_LEN 47

/*--------------------------------------------------------------------------------------------
                    NAV CONSTANTS AND GLOBAL VARIABLES
--------------------------------------------------------------------------------------------*/

// Defines the nav task period to 20 ms (50 Hz)
#define NAV_PERIOD    20    

// Standard sampling period time
#define A_MILLI_SECOND 1000000

// Real Time task default priority
#define NAV_TASK_PRIORITY 1

// NAV Real Time Task Global Variable
RT_TASK task_nav;

// NAV data Global Variable
msg_nav_t global_msg_nav;

// Variavel global para avisar ao slave sobre um dado novo
int global_new_data_nav = 0;

/*--------------------------------------------------------------------------------------------
                    NAV FUNCTIONS
--------------------------------------------------------------------------------------------*/
// Opens the NAV communication and configures it
int rt_open_nav(void);

// Main function executed by the NAV real time task
void func_nav(int t);

//Convert the message received (msgbuf) to engineering units (msg)
int rt_convert_nav_data(msg_nav_t* msg,unsigned char* msgbuf);

//Gets a data packet
int rt_process_nav_serial(unsigned char* MessageBuffer);

//calculates the msg crc and returns it
//it is used both for sending and receiving msgs
unsigned int rt_crc_calc(unsigned char* MessageBuffer);

//checks the message crc and returns 1 (correct) or 0 (wrong)
//crc is available on the msg's 2 last bytes
int rt_crc_check(unsigned char* MessageBuffer);

//Allows the other module (fdc_slave) to get the nav data
//The function returns 1 for new data and 0 for old data
int rt_get_nav_data(msg_nav_t *msg);

#endif

