/*
    rtai_gps.c - garmin GPS-18x-5Hz real time data acquisition module
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

/**************************************************
Uses some functions developed by:

    Flávio de Deus Mota, Armando Alves Neto and 
    Leonardo A. B. Torres
***************************************************/

#include "rtai_gps.h"
#include <linux/kernel.h> //needed to use sprintf

MODULE_AUTHOR("Victor Costa da Silva Campos");
MODULE_DESCRIPTION("Real time data acquisition of garmin GPS18x-5Hz");
MODULE_LICENSE("GPL");

// Sends a GPS command over the serial
void rt_sendGPScommand(const char *command)
{    
    char chksum_str[3];
    int checksum = 0;
    int k;
    //Calculates the checksum
        // XOR sum of the characters
        for (k=0;command[k] != '\0';k++)
            checksum = checksum^command[k];
        // get the hexadecimal representation
        sprintf(chksum_str,"%2X",checksum);
    //Send the message over the serial
        rt_putch_serial(GPS_PORT,'$');
        for (k=0;command[k] != '\0';k++) {
            rt_putch_serial(GPS_PORT,command[k]);
        }
        rt_putch_serial(GPS_PORT,'*');
        rt_putch_serial(GPS_PORT,chksum_str[0]);
        rt_putch_serial(GPS_PORT,chksum_str[1]);
        rt_putch_serial(GPS_PORT,'\r'); //<CR>
        rt_putch_serial(GPS_PORT,'\n'); //<LF>
};

// checks the message checksum - returns 1 for ok and 0 for error
int checksum(const char* msg)
{
    int k,check;
    char chksum_str[3];
    //Goes through the array
    check = 0;
    for (k=0;msg[k] != '*';++k)
        check ^= msg[k];
    //Change it to hexa    
    sprintf(chksum_str,"%2X",check);
    if ((msg[k+1]==chksum_str[0])&&(msg[k+2]==chksum_str[1]))
        return 1;
    else
        return 0;
}

// Asks for a GPS reset
void rt_request_gps_reset(void)
{
    // Asks for a GPS reset
    global_reset_GPS = GPS_RESET;
}

// get the gps data
int rt_get_gps_data(msg_gps_t *d)
{
    //gga messages
    d->latitude = global_msg_gps.latitude;
    d->longitude = global_msg_gps.longitude;
    d->altitude = global_msg_gps.altitude;
    d->hdop = global_msg_gps.hdop;
    d->geoid_separation = global_msg_gps.geoid_separation;
    d->north_south = global_msg_gps.north_south;
    d->east_west = global_msg_gps.east_west;
    d->fix_indicator = global_msg_gps.fix_indicator;
    d->n_satellites = global_msg_gps.n_satellites;
    d->units_altitude = global_msg_gps.units_altitude;
    d->units_geoid_separation = global_msg_gps.units_geoid_separation;
    d->GPS_time_gga = global_msg_gps.GPS_time_gga;
    //rmc messages
    d->GPS_time_rmc = global_msg_gps.GPS_time_rmc;
    d->status = global_msg_gps.status;
    d->gspeed = global_msg_gps.gspeed;
    d->course = global_msg_gps.course;
    d->date = global_msg_gps.date;
    d->magvar = global_msg_gps.magvar;
    d->magvardir = global_msg_gps.magvardir;
    d->mode = global_msg_gps.mode;
    //rme messages
    d->hpe = global_msg_gps.hpe;
    d->vpe = global_msg_gps.vpe;
    d->epe = global_msg_gps.epe;
    d->hpe_units = global_msg_gps.hpe_units;
    d->vpe_units = global_msg_gps.vpe_units;
    d->epe_units = global_msg_gps.epe_units;
    //rmv messages
    d->east_v = global_msg_gps.east_v;
    d->north_v = global_msg_gps.north_v;
    d->up_v = global_msg_gps.up_v;
    //rmc messages
    d->validity = global_msg_gps.validity;
    
    return 0; 
}

//Resets the GPS desired messages configuration
void rt_reset_gps(void)
{
    // Change the reset_GPS variable state
    global_reset_GPS = GPS_NO_RESET;

    //Sends over the configuration msgs
    //for now just the message configuration ones
    rt_sendGPScommand(NMEA_NO_MSG);
    rt_sendGPScommand(NMEA_RMC);
    rt_sendGPScommand(NMEA_GGA);
    rt_sendGPScommand(NMEA_PGRME);
    rt_sendGPScommand(NMEA_PGRMV);
    rt_sendGPScommand(RESET_MSG);
}

//Opens the GPS communication and configures it
int rt_open_gps(void) { 

    // Inits the msg data structure
    global_msg_gps.latitude = 0;
    global_msg_gps.longitude = 0;
    global_msg_gps.altitude = 0;
    global_msg_gps.hdop = 0;
    global_msg_gps.geoid_separation = 0;
    global_msg_gps.east_west = 0;
    global_msg_gps.units_altitude = 0;
    global_msg_gps.units_geoid_separation = 0;
    global_msg_gps.north_south = 0;
    global_msg_gps.validity = 0;
    global_msg_gps.fix_indicator = 0;
    global_msg_gps.n_satellites = 0;
    global_msg_gps.GPS_time_gga = 0;
    global_msg_gps.GPS_time_rmc = 0;
    global_msg_gps.status = 0;
    global_msg_gps.gspeed = 0;
    global_msg_gps.course = 0;
    global_msg_gps.date = 0;
    global_msg_gps.magvar = 0;
    global_msg_gps.magvardir = 0;
    global_msg_gps.mode = 0;

    //Says that we do not wish to reset the GPS
    global_reset_GPS = GPS_NO_RESET;    

    //Opens the GPS serial port
    if (rt_open_serial(GPS_PORT,38400) < 0) {
        rt_printk("Nao abriu a serial do GPS\n");
                 return -1; // Error
        }
    else {
        rt_printk("Abriu a serial do GPS\n");
        return 0;
    };
};

//Transforms chars in a float (useful for some data)
//Also updates the index to after the expected comma
float char2float(unsigned char* caracs, unsigned int* begin) {
    int sign;
    unsigned int index = *begin;
    float number = 0;
    //checks the number sign
    if (caracs[index] == '-') {
        sign = -1;
        ++index;
    }
    else sign = 1;
    //looks for the dot
    int delta_dot = 0;
    float mult = 1;
    while (caracs[index+delta_dot] != '.') ++delta_dot;
    int i;
    for (i = (delta_dot-1); i >= 0; --i) {
        //adds the new char
        number += mult*C2I(caracs[index+i]);
        //updates multiplier for next iteration
        mult = mult*10;
    }
    //jumps the index to one step after the dot
    index += (delta_dot+1);
    //resets the multiplier
    mult = 1;
    while ((caracs[index] != ',')&&(caracs[index] != '*')) {
        mult = 0.1*mult;
        number += mult*C2I(caracs[index]);
        ++index;
    };
    //the current index should be a comma or an *, so jump one more time
    ++index;
    *begin = index;
    number = sign*number;
    return number;
};


//Parses incoming message and stores relevant data in gps_msg structure
void rt_parse_msg(unsigned char* msgbuf) {
    unsigned int msgIndex = 5;
    //checks if its a gga msg
    if ((msgbuf[0]=='G')&&(msgbuf[1]=='P')&&(msgbuf[2]=='G')&&(msgbuf[3]=='G')&&(msgbuf[4]=='A')) {
        //the next char should be a comma, else abort
        if (msgbuf[msgIndex] != ',') return;
        ++msgIndex; //go to the next one
        //check if we have the UTC time value or not
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.GPS_time_gga = 0;
            ++msgIndex;
        }
        else {
            int hour = 10*C2I(msgbuf[msgIndex])+C2I(msgbuf[msgIndex+1]);
            int minute = 10*C2I(msgbuf[msgIndex+2])+C2I(msgbuf[msgIndex+3]);
            float second = 10*C2I(msgbuf[msgIndex+4])+C2I(msgbuf[msgIndex+5])+0.1*C2I(msgbuf[msgIndex+7]);
            global_msg_gps.GPS_time_gga = 3600*hour + 60*minute + second;
            msgIndex += 9; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the latitude value or not
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.latitude = 0;
            ++msgIndex;
        }
        else {
            int deg = 10*C2I(msgbuf[msgIndex])+C2I(msgbuf[msgIndex+1]);//degress
            float minute = 10*C2I(msgbuf[msgIndex+2])+C2I(msgbuf[msgIndex+3]);//minutes
            minute += 0.1*C2I(msgbuf[msgIndex+5])+0.01*C2I(msgbuf[msgIndex+6])+0.001*C2I(msgbuf[msgIndex+7])+0.0001*C2I(msgbuf[msgIndex+8])+0.00001*C2I(msgbuf[msgIndex+9]);
            global_msg_gps.latitude = deg + (minute / 60);
            msgIndex += 11; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the hemisphere value or not
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.north_south = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.north_south = msgbuf[msgIndex];
            msgIndex += 2; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the longitude value or not
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.longitude = 0;
            ++msgIndex;
        }
        else {
            int deg = 100*C2I(msgbuf[msgIndex])+10*C2I(msgbuf[msgIndex+1])+C2I(msgbuf[msgIndex+2]);//degress
            float minute = 10*C2I(msgbuf[msgIndex+3])+C2I(msgbuf[msgIndex+4]);//minutes
            minute += 0.1*C2I(msgbuf[msgIndex+6])+0.01*C2I(msgbuf[msgIndex+7])+0.001*C2I(msgbuf[msgIndex+8])+0.0001*C2I(msgbuf[msgIndex+9])+0.00001*C2I(msgbuf[msgIndex+10]);
            global_msg_gps.longitude = deg + (minute / 60);
            msgIndex += 12; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the hemisphere value or not
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.east_west = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.east_west = msgbuf[msgIndex];
            msgIndex += 2; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the fix indicator
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.fix_indicator = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.fix_indicator = msgbuf[msgIndex];
            msgIndex += 2; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the number of satellites
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.n_satellites = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.n_satellites = 10*C2I(msgbuf[msgIndex]) + C2I(msgbuf[msgIndex+1]);
            msgIndex += 3; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have horizontal dillution
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.hdop = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.hdop = char2float(msgbuf,&msgIndex);
        };
        //check if we have antenna height
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.altitude = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.altitude = char2float(msgbuf,&msgIndex);
        };
        //check if we have altitude units
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.units_altitude = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.units_altitude = msgbuf[msgIndex];
            msgIndex += 2;
        };
        //check if we have geoidal height
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.geoid_separation = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.geoid_separation = char2float(msgbuf,&msgIndex);
        };
        //check if we have geoidal height units
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.units_geoid_separation = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.units_geoid_separation = msgbuf[msgIndex];
            msgIndex += 2;
        };
        //And the rest does not matter for now (dgps data)
    }
    //checks if its a rmc msg
    if ((msgbuf[0]=='G')&&(msgbuf[1]=='P')&&(msgbuf[2]=='R')&&(msgbuf[3]=='M')&&(msgbuf[4]=='C')) {
        //the next char should be a comma, else abort
        if (msgbuf[msgIndex] != ',') return;
        ++msgIndex; //go to the next one
        //check if we have the UTC time value or not
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.GPS_time_rmc = 0;
            ++msgIndex;
        }
        else {
            int hour = 10*C2I(msgbuf[msgIndex])+C2I(msgbuf[msgIndex+1]);
            int minute = 10*C2I(msgbuf[msgIndex+2])+C2I(msgbuf[msgIndex+3]);
            float second = 10*C2I(msgbuf[msgIndex+4])+C2I(msgbuf[msgIndex+5])+0.1*C2I(msgbuf[msgIndex+7]);
            global_msg_gps.GPS_time_rmc = 3600*hour + 60*minute + second;
            msgIndex += 9; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the signal status
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.status = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.status = msgbuf[msgIndex];
            msgIndex += 2; //jumps to the next value field (also jumps de comma -> +1)
        };    
        if (global_msg_gps.status == 65) {
           global_msg_gps.validity = 1;
        } 
        //check if we have the latitude value or not // This value isn't been used because we already get it from GGA message
        if (msgbuf[msgIndex] == ',') {
            //global_msg_gps.latitude = 0;
            ++msgIndex;
        }
        else {
            //int deg = 10*C2I(msgbuf[msgIndex])+C2I(msgbuf[msgIndex+1]);//degress
            //float minute = 10*C2I(msgbuf[msgIndex+2])+C2I(msgbuf[msgIndex+3]);//minutes
            //minute += 0.1*C2I(msgbuf[msgIndex+5])+0.01*C2I(msgbuf[msgIndex+6])+0.001*C2I(msgbuf[msgIndex+7])+0.0001*C2I(msgbuf[msgIndex+8])+0.00001*C2I(msgbuf[msgIndex+9]);
            //global_msg_gps.latitude = deg + (minute / 60); 
            msgIndex += 11; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the hemisphere value or not // This value isn't been used because we already get it from GGA message
        if (msgbuf[msgIndex] == ',') {
            //global_msg_gps.north_south = 0;
            ++msgIndex;
        }
        else {
            //global_msg_gps.north_south = msgbuf[msgIndex];
            msgIndex += 2; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the longitude value or not // This value isn't been used because we already get it from GGA message
        if (msgbuf[msgIndex] == ',') {
            //global_msg_gps.longitude = 0;
            ++msgIndex;
        }
        else {
            //int deg = 100*C2I(msgbuf[msgIndex])+10*C2I(msgbuf[msgIndex+1])+C2I(msgbuf[msgIndex+2]);//degress
            //float minute = 10*C2I(msgbuf[msgIndex+3])+C2I(msgbuf[msgIndex+4]);//minutes
            //minute += 0.1*C2I(msgbuf[msgIndex+6])+0.01*C2I(msgbuf[msgIndex+7])+0.001*C2I(msgbuf[msgIndex+8])+0.0001*C2I(msgbuf[msgIndex+9])+0.00001*C2I(msgbuf[msgIndex+10]);
            //global_msg_gps.longitude = deg + (minute / 60);
            msgIndex += 12; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have the hemisphere value or not // This value isn't been used because we already get it from GGA message
        if (msgbuf[msgIndex] == ',') {
            //global_msg_gps.east_west = 0;
            ++msgIndex;
        }
        else {
            //global_msg_gps.east_west = msgbuf[msgIndex];
            msgIndex += 2; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have speed over grounf
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.gspeed = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.gspeed = char2float(msgbuf,&msgIndex);
        };
        //check if we have course
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.course = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.course = char2float(msgbuf,&msgIndex);
        };
        //check if we have the UTC date fix
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.date = 0;
            ++msgIndex;
        }
        else {
            int day = 10*C2I(msgbuf[msgIndex])+C2I(msgbuf[msgIndex+1]);
            int month = 10*C2I(msgbuf[msgIndex+2])+C2I(msgbuf[msgIndex+3]);
            int year = 10*C2I(msgbuf[msgIndex+4])+C2I(msgbuf[msgIndex+5]);
            global_msg_gps.date = day*10000 + month*100 + year;
            msgIndex += 7; //jumps to the next value field (also jumps de comma -> +1)
        };
        //check if we have magnetic variation
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.magvar= 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.magvar = char2float(msgbuf,&msgIndex);
        };
        //check if we have the magnetic variation direction
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.magvardir = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.magvardir = msgbuf[msgIndex];
            msgIndex += 2; //jumps to the next value field (also jumps de comma -> +1)
        };        
        //check if we have the mode indicator
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.mode = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.mode = msgbuf[msgIndex];
            msgIndex += 2; //jumps to the next value field (also jumps de comma -> +1)
        };        
        //And the rest does not matter for now (dgps data)
    }
    //checks if its a rmv msg
    if ((msgbuf[0]=='P')&&(msgbuf[1]=='G')&&(msgbuf[2]=='R')&&(msgbuf[3]=='M')&&(msgbuf[4]=='V')) {
        //the next char should be a comma, else abort
        if (msgbuf[msgIndex] != ',') return;
        ++msgIndex; //go to the next one
        //check if we have true east velocity
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.east_v = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.east_v = char2float(msgbuf,&msgIndex);
        };
        //check if we have true north velocity
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.north_v = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.north_v = char2float(msgbuf,&msgIndex);
        };
        //check if we have up velocity
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.up_v = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.east_v = char2float(msgbuf,&msgIndex);
        };
    }
    //checks if its a rme msg
    if ((msgbuf[0]=='P')&&(msgbuf[1]=='G')&&(msgbuf[2]=='R')&&(msgbuf[3]=='M')&&(msgbuf[4]=='E')) {
        //the next char should be a comma, else abort
        if (msgbuf[msgIndex] != ',') return;
        ++msgIndex; //go to the next one                                                                                                  
        //check if we have horizontal position error
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.hpe = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.hpe = char2float(msgbuf,&msgIndex);
        };
        //check if we have horizontal position error units
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.hpe_units = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.hpe_units = msgbuf[msgIndex];
            msgIndex += 2;
        };
        //check if we have vertical position error
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.vpe = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.vpe = char2float(msgbuf,&msgIndex);
        };
        //check if we have vertical position error units
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.vpe_units = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.vpe_units = msgbuf[msgIndex];
            msgIndex += 2;
        };
        //check if we have estimated position error
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.epe = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.epe = char2float(msgbuf,&msgIndex);
        };
        //check if we have estimated position error units
        if (msgbuf[msgIndex] == ',') {
            global_msg_gps.epe_units = 0;
            ++msgIndex;
        }
        else {
            global_msg_gps.epe_units = msgbuf[msgIndex];
            msgIndex += 2;
        };
    }
};

//Processes incoming serial GPS messages
void rt_process_gps_serial(unsigned char* msgbuf){
    //finite state status
    unsigned char state = 0;
    //received char data variable
    int ch;
    //msg buffer "pointer"
    unsigned int msgIndex = 0;

    //while we still have data on the serial buffer
    while (rt_bytes_avail_serial(GPS_PORT)) // Evaluates if there are available bytes
    {
        //Get a byte from the serial port.
        ch = rt_getch_serial(GPS_PORT);
        ch=ch&0xFF;

        //finite state machine
        switch(state) {
            case 0: //looking for msg header -> $
                if(ch == GPS_HEADER) {
                    state++; //go to the next state
                    msgIndex = 0; //reset the msg index
                };
            break;
            case 1: //Filling up the message
                if(ch != 0x0d) { //if it's not the end of the message
                    msgbuf[msgIndex++] = ch; //saves the read byte
                }
                else { //if it's the end of the message
                    if (checksum(msgbuf)) {//if the checksum is valid
                        rt_parse_msg(msgbuf); // parses the received message
                    };
                    state = 0; //resets the state machine
                    //could go to a state looking for the next end char
                    ///but that's not really necessary
                };
            break;
        };//end switch
    };
};

// Main function executed by the GPS real time task
void func_gps(int t) {
    unsigned char msgbuf[GPS_MSG_LEN]; //buffer for receiving the message
        
    while (1) { // while the modules doesn't terminates        
        // Tries to process incoming messages
        rt_process_gps_serial(msgbuf);

        //checks if someone asked for a gps reset
        if(global_reset_GPS == GPS_RESET) rt_reset_gps();
                        
        //Waits for 200ms (5Hz)
        rt_task_wait_period();
    }

    return (void)0;
};

// GPS module's initializer
static int __rtai_gps_init(void)
{

    RTIME tick_period;    // Determines the task's period
    RTIME now;        // current time in ns
    
    // Opens the GPS communication and configures it
    if (rt_open_gps() < 0) {
        rt_printk("Nao abriu o dispositivo GPS\n");
        return -1;
        }

    // Creates the real time task
    if (rt_task_init(&task_gps, func_gps, 0, 5000, GPS_TASK_PRIORITY, 0, 0) < 0) {
        rt_printk("Falha ao criar a tarefa de tempo real do GPS\n");
        return -1;
    }
    
    // Determines the task execution period as being a 1 ms multiple (PERIOD* 1 ms)
    tick_period = GPS_PERIOD*start_rt_timer(nano2count(A_MILLI_SECOND));
    now = rt_get_time();
    
    //Launches the main task as a periodic task
    if (rt_task_make_periodic(&task_gps, now + tick_period, tick_period) < 0) {
        rt_printk("Nao consegui lancar tarefa de tempo real periodicamente\n");
                return -1;
    }
    
    return 0;
};

// GPS module's destructor
static void __rtai_gps_cleanup(void)
{
    //Terminates the real time task
    rt_task_delete(&task_gps);
    //Clears the serial buffer
    if (rt_clear_serial(GPS_PORT) == 0)
        rt_printk("Apagou buffer da serial (GPS) com sucesso\n");
    else
        rt_printk("Não conseguiu apagar buffer da serial (GPS)\n");
    //Closes the serial port
    if (rt_close_serial(GPS_PORT) == 0)
        rt_printk("Fechou a serial do GPS\n");
    else
        rt_printk("Não conseguiu fechar a serial do GPS\n");    
};


module_init(__rtai_gps_init);
module_exit(__rtai_gps_cleanup);

