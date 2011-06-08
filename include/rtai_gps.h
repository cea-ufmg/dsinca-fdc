/*
	rtai_gps.h - garmin GPS-18x-5Hz module header
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

#ifndef RTAI_GPS_H
#define RTAI_GPS_H

#include "rtai_rt_serial.h"
#include "messages.h"

//Desired messages commands
#define NMEA_ALL_MSG "PGRMO,,3"
#define NMEA_NO_MSG "PGRMO,,2"

#define NMEA_RMC "PGRMO,GPRMC,1"
#define NMEA_NO_RMC "PGRMO,GPRMC,0"

#define NMEA_GGA "PGRMO,GPGGA,1"
#define NMEA_NO_GGA "PGRMO,GPGGA,0"

#define NMEA_GSA "PGRMO,GPGSA,1"
#define NMEA_NO_GSA "PGRMO,GPGSA,0"

#define NMEA_GSV "PGRMO,GPGSV,1"
#define NMEA_NO_GSV "PGRMO,GPGSV,0"

#define NMEA_PGRME "PGRMO,PGRME,1"
#define NMEA_NO_PGRME "PGRMO,PGRME,0"

#define NMEA_GLL "PGRMO,GPGLL,1"
#define NMEA_NO_GLL "PGRMO,GPGLL,0"

#define NMEA_VTG "PGRMO,GPVTG,1"
#define NMEA_NO_VTG "PGRMO,GPVTG,0"

#define NMEA_PGRMV "PGRMO,PGRMV,1"
#define NMEA_NO_PGRMV "PGRMO,PGRMV,0"

#define NMEA_PGRMF "PGRMO,PGRMF,1"
#define NMEA_NO_PGRMF "PGRMO,PGRMF,0"

#define NMEA_PGRMB "PGRMO,PGRMB,1"
#define NMEA_NO_PGRMB "PGRMO,PGRMB,0"

#define NMEA_PGRMM "PGRMO,PGRMM,1"
#define NMEA_NO_PGRMM "PGRMO,PGRMM,0"

#define NMEA_PGRMT "PGRMO,PGRMT,1"
#define NMEA_NO_PGRMT "PGRMO,PGRMT,0"

#define RESET_MSG "PGRMI,,,,,,,R"

//Since we'll be using GGA, RMC, PGRMV and PGRME define
//their identifiers because it could come in handy
#define GGA_ID "GPGGA"
#define RMC_ID "GPRMC"
#define PGRMV_ID "PGRMV"
#define PGRME_ID "PGRME"

//Define the GPS header
#define GPS_HEADER '$'
//Define the GPS end message chars
#define GPS_END1 '/r'
#define GPS_END2 '/n'

//constants
//baud rate
#define GPS_DEFAULT_BAUD 38400

//sampling period (ms)
#define GPS_PERIOD 200

// Standard sampling period time
#define A_MILLI_SECOND 1000000

// Real Time task default priority
#define GPS_TASK_PRIORITY 2

// max len of received messages
#define GPS_MSG_LEN 82

// GPS Real Time Task Global Variable
RT_TASK task_gps;

// GPS data Global Variable
msg_gps_t global_msg_gps;

//global reset variable
int global_reset_GPS;
//global ready data variable
int DataReady;

//GPS message and status constants
#define GPS_DATA_READY 1
#define GPS_NO_DATA_READY 0

#define GPS_NO_RESET 0
#define GPS_RESET 1

#define GPS_VALID_DATA 1
#define GPS_INVALID_DATA 0
#define GPS_TIMEOUT_FAILURE 2

//ASCII to integer conversion define
#define C2I(carac) (((int)carac)-48)

//GPS functions

// Main function executed by the GPS real time task
void func_gps(int t);
//Processes incoming serial GPS messages
void rt_process_gps_serial(unsigned char* msgbuf);
//Parses incoming message and stores relevant data in gps_msg structure
void rt_parse_msg(unsigned char* msgbuf);
//Transforms chars in a float (useful for some data)
//Also updates the index to after the expected comma
float char2float(unsigned char* caracs, unsigned int* index);
//Opens the GPS communication and configures it
int rt_open_gps(void);
//Resets the GPS desired messages configuration
void rt_reset_gps(void);
// get the gps data
int rt_get_gps_data(msg_gps_t *d);
// Asks for a GPS reset
void rt_request_gps_reset(void);
// checks the message checksum - returns 1 for ok and 0 for error
int checksum(const char* msg);
// Sends a GPS command over the serial
void rt_sendGPScommand(const char *command);

#endif
