/*
    rtai_nav.h - wireless pitot tube module header
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

#ifndef RTAI_PITOT_H
#define RTAI_PITOT_H

#include "rtai_rt_serial.h"
#include "messages.h"

//Define PITOT message's constants
//The header is composed of 0x5555 (UU) (repeat the PITOT_HEADER_CHAR)
#define PITOT_HEADER_CHAR 0x55 // U


/*--------------------------------------------------------------------------------------------
                    PITOT DATA CONVERSION
--------------------------------------------------------------------------------------------*/

//Conversion macros
//Temperature Conversion - input int_16 / output double in ºC
#define PITOT_RAW2TEMP(Traw) (Traw*0.05)

//Static pressure conversion - input int_32t / output double in Pa
#define PITOT_RAW2STATIC(Praw) (Praw/4.0)

//Dynamic pressure conversion - input int_16 / output double in Pa
#define PITOT_RAW2DYNAMIC(Praw) (Praw)

//Angle of attack conversion - input int_16 / output double in 
#define PITOT_RAW2ALPHA(Araw) (Araw)

//Sideslip angle conversion - input int_16 / output double in
#define PITOT_RAW2BETA(Braw) (Braw)

//Conversion functions
//Transform a 2-complement temperature data to a common int_16
int convert_temp_int_data(unsigned char msb,unsigned char lsb);

/*--------------------------------------------------------------------------------------------
                    PITOT COMMUNICATION DEFAULTS

    RS-232
    baud rate - 9600 (default)
    8 data bits
    1 start bit
    1 stop bit
    no parity
    no handshake (flow control)
--------------------------------------------------------------------------------------------*/
#define PITOT_DEFAULT_BAUD 9600
#define PITOT_MSG_LEN 11

/*--------------------------------------------------------------------------------------------
                    PITOT CONSTANTS AND GLOBAL VARIABLES
--------------------------------------------------------------------------------------------*/

// Defines the pitot task period to 20 ms (50 Hz)
#define PITOT_PERIOD    20    

// Standard sampling period time
#define A_MILLI_SECOND 1000000

// Real Time task default priority
#define PITOT_TASK_PRIORITY 1

// PITOT Real Time Task Global Variable
RT_TASK task_pitot;

// PITOT data Global Variable
msg_pitot_t global_msg_pitot;

// Variavel global para avisar ao slave sobre um dado novo
int global_new_data_pitot = 0;

/*--------------------------------------------------------------------------------------------
                    PITOT FUNCTIONS
--------------------------------------------------------------------------------------------*/
// Opens the PITOT communication and configures it
int rt_open_pitot(void);

// Main function executed by the PITOT real time task
void func_pitot(int t);

//Convert the message received (msgbuf) to engineering units (msg)
int rt_convert_pitot_data(msg_pitot_t* msg,unsigned char* msgbuf);

//Gets a data packet
int rt_process_pitot_serial(unsigned char* MessageBuffer);

//Allows the other module (fdc_slave) to get the pitot data
//The function returns 1 for new data and 0 for old data
int rt_get_pitot_data(msg_pitot_t *msg);

#endif

