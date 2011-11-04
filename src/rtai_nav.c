/*
    rtai_nav.c - NAV440CA-400 real time data acquisition module
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

#include "rtai_nav.h"

MODULE_AUTHOR("Victor Costa da Silva Campos");
MODULE_DESCRIPTION("Real time data acquisition of xbow NAV440CA-400");
MODULE_LICENSE("GPL");

/*--------------------------------------------------------------------------------------------
                    NAV FUNCTIONS
--------------------------------------------------------------------------------------------*/

// Opens the NAV communication
int rt_open_nav(void) 
{

    // Opens the NAV serial port for communication
    if (rt_open_serial(NAV_PORT,NAV_DEFAULT_BAUD) < 0) {
        rt_printk("Nao abriu a serial do NAV\n");
                 return -1; // Error
        }
        rt_printk("Abriu a porta serial do NAV\n");
        return 0;
}

// Configures the NAV
int rt_cfg_nav(void)
{
    //Sets the NAV to quiet mode so it doesn't fill us with messages
    //while we're trying to configure it
    
    //Discards available messages
    
    //Tries to ping the device and
    //check if we got the right answer

    //Gets the NAV version and print it on the screen

    // Tries to config the NAV 10 times, if it fails, abort execution
    int count_try;
    for (count_try=0; count_try<10; ++count_try) {
    };
    //checks whether or not we were succesful
    if (count_try >= 10) {
        printk("[rt_cfg_nav]: não consegui configurar NAV. ");
        return -1; //couldn't config the NAV
    };

    //if needed insert here the code to change the nav baud rate
    //though it should be noted that the default is 38400
    //and only 12000 are needed for the 50Hz (8*30*50)
    
    //Sets the NAV packet rate
    //Discards available messages

    // Tries to set the NAV packet rate 10 times, if it fails, abort execution
    for (count_try=0; count_try<10; ++count_try) {
    };
    //checks whether or not we were succesful
    if (count_try >= 10) {
        printk("[rt_cfg_nav]: não consegui configurar o NAV");
        return -1; //couldn't set the packet rate
    }
    else {
        return 0; //success
    };
};

// Main function executed by the NAV real time task
void func_nav(int t)
{
    unsigned char msgbuf[NAV_MSG_LEN]; //buffer for receiving the message
        
    while (1) { // while the modules doesn't terminates        
        // Tries to process incoming messages
        global_msg_nav.validade = rt_process_nav_serial(msgbuf);
                    
        // if it is a valid message, updates the global variable
        if (global_msg_nav.validade == 1)
        {
            //Sets the new data variable
            global_new_data_nav = rt_convert_nav_data(&global_msg_nav, msgbuf);
        }
        
        //Waits for 20ms (50Hz)
        rt_task_wait_period();
    }

    return (void)0;
};

//Transform a 2-complement word (2 bytes) to a common int
int convert_int_data(unsigned char msb,unsigned char lsb){
    unsigned short word = (msb << 8)|lsb; //bigger than needed to avoid trouble
    int data;
    if (word & 0x8000) {
        word ^= 0xFFFF; //XOR
        word++;
        data = -word;
    }
    else {
        data = word;
    }
    return data;
};

//Transform a 2-complement double word (4 bytes) to a common int
int convert_int_data2(unsigned char msb1,unsigned char lsb1, unsigned char msb2, unsigned char lsb2){
    unsigned short word1 = (msb1 << 8)|lsb1; //bigger than needed to avoid trouble
    unsigned short word2 = (msb2 << 8)|lsb2; //bigger than needed to avoid trouble
    unsigned int dword = (word1 << 16)|word2;
    int data;
    if (dword & 0x80000000) {
        dword ^= 0xFFFFFFFF; //XOR
        dword++;
        data = -dword;
    }
    else {
        data = dword;
    }
    return data;
};

//Convert the message received (msgbuf_in) to engineering units (msg)
int rt_convert_nav_data(msg_nav_t* msg,unsigned char* msgbuf_in)
{
    unsigned char* msgbuf = msgbuf_in + 3;
    int16_t aux;
    uint16_t u_aux;

    int32_t aux2;
    uint32_t u_aux2;

    //Euler angles (estimated by the NAV)
    aux = convert_int_data(msgbuf[0],msgbuf[1]);
    msg->angle[0] = (float)NAV_RAW2ANGLE(aux);
    aux = convert_int_data(msgbuf[2],msgbuf[3]);
    msg->angle[1] = (float)NAV_RAW2ANGLE(aux);
    aux = convert_int_data(msgbuf[4],msgbuf[5]);
    msg->angle[2] = (float)NAV_RAW2ANGLE(aux);

    //Angular Rates
    aux = convert_int_data(msgbuf[6],msgbuf[7]);
    msg->gyro[0] = (float)NAV_RAW2RATE(aux);
    aux = convert_int_data(msgbuf[8],msgbuf[9]);
    msg->gyro[1] = (float)NAV_RAW2RATE(aux);
    aux = convert_int_data(msgbuf[10],msgbuf[11]);
    msg->gyro[2] = (float)NAV_RAW2RATE(aux);

    //Accelarations
    aux = convert_int_data(msgbuf[12],msgbuf[13]);
    msg->accel[0] = (float)NAV_RAW2ACCEL(aux);
    aux = convert_int_data(msgbuf[14],msgbuf[15]);
    msg->accel[1] = (float)NAV_RAW2ACCEL(aux);
    aux = convert_int_data(msgbuf[16],msgbuf[17]);
    msg->accel[2] = (float)NAV_RAW2ACCEL(aux);

    //Velocities
    aux = convert_int_data(msgbuf[18],msgbuf[19]);
    msg->nVel = (float)NAV_RAW2VEL(aux);
    aux = convert_int_data(msgbuf[20],msgbuf[21]);
    msg->eVel = (float)NAV_RAW2VEL(aux);
    aux = convert_int_data(msgbuf[22],msgbuf[23]);
    msg->dVel = (float)NAV_RAW2VEL(aux);

    //Longitude
    //aux2 = convert_int_data2(msgbuf[24],msgbuf[25],msgbuf[26],msgbuf[27]);
    aux2 = (msgbuf[24]<<24)|(msgbuf[25]<<16)|(msgbuf[26]<<8)|msgbuf[27];
    msg->longitude = (float)NAV_RAW2LAT(aux2);

    //Latitude
    //aux2 = convert_int_data2(msgbuf[28],msgbuf[29],msgbuf[30],msgbuf[31]);
    aux2 = (msgbuf[28]<<24)|(msgbuf[29]<<16)|(msgbuf[30]<<8)|msgbuf[31];
    msg->latitude = (float)NAV_RAW2LAT(aux2);

    //Altitude
    aux = convert_int_data(msgbuf[32],msgbuf[33]);
    msg->altitude = (float)NAV_RAW2ALT(aux);
    
    //Temperature Voltage
    u_aux = convert_int_data(msgbuf[34],msgbuf[35]);
    msg->temp = (float)NAV_RAW2TEMP(u_aux);

    //Internal Time
    u_aux2 = (msgbuf[36]<<24)|(msgbuf[37]<<16)|(msgbuf[38]<<8)|msgbuf[39];
    msg->time_stamp = u_aux2;

    //Internal status mask
    msg->internal_status = msgbuf[40];
    //Internal error mask
    msg->internal_error = msgbuf[41];

    return 1;
};

//Gets a data packet
int rt_process_nav_serial(unsigned char* MessageBuffer)
{
    static unsigned char state = 0;      // binary state variable (0 -> waiting for header/ 1 -> filling message)
    unsigned char ch;            // Current byte in the serial port

    static unsigned char MessageIndex = 0;            // Current message index

    while (rt_bytes_avail_serial(NAV_PORT)) // Checks if there are data available
    {
        //Get a byte from the serial port.
        ch = rt_getch_serial(NAV_PORT);
        ch=ch&0xFF;

        switch (state)
        {
            case 0:  //Look for the 1st header char
                if (ch == NAV_HEADER_CHAR) state++; //we found it, so we get the rest of the message
            break;
            /////////////////////////////////////////////////////////////////////////
            case 1:  //Look for the 2nd header char
                if (ch == NAV_HEADER_CHAR) state++; //we found it, so we get the rest of the message
                else state = 0;
            break;
            /////////////////////////////////////////////////////////////////////////
            case 2: //Look for the first package type char
                if (ch == NAV_PACK) {
                    state++;
                    MessageBuffer[MessageIndex++] = ch; //Save the byte
                }
                else state = 0;
            break;
            /////////////////////////////////////////////////////////////////////////
            case 3: //Look for the second package type char
                if (ch == NUM1) {
                    state++;
                    MessageBuffer[MessageIndex++] = ch; //Save the byte
                }
                else {
                    state = 0;
                    MessageIndex = 0;
                }
            break;
            /////////////////////////////////////////////////////////////////////////
            case 4: //Look for the second package type char
                if (ch == 42) {
                    state++;
                    MessageBuffer[MessageIndex++] = ch; //Save the byte
                }
                else {
                    state = 0;
                    MessageIndex = 0;
                }
            break;
            /////////////////////////////////////////////////////////////////////////
            case 5:  //Fill the message buffer
                MessageBuffer[MessageIndex++] = ch; //Save the byte
                //checks to see if we completed the message
                if (MessageIndex == NAV_MSG_LEN) {
                    MessageIndex = 0; state = 0; //resets the finite state machine
                    //checks the crc and returns 0 (failure) or 1 (success)
                    return rt_crc_check(MessageBuffer);
                }
            break;
        };
    };
    return -1;    // No available data
};

//calculates the msg crc and returns it
//it is used both for sending and receiving msgs
unsigned int rt_crc_calc(unsigned char* MessageBuffer) {
    unsigned int i = 0, j = 0;
    unsigned short crc = 0x1D0F;

    for (i=0; i<NAV_MSG_LEN-2; ++i) {
        crc ^= MessageBuffer[i] << 8;
        for (j=0; j<8; ++j) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc = crc << 1;
        }
    }

    return crc;
};

//checks the message crc and returns 1 (correct) or 0 (wrong)
//crc is available on the msg's 2 last bytes
int rt_crc_check(unsigned char* MessageBuffer){
    unsigned int crc = rt_crc_calc(MessageBuffer);
    if ((((crc&0xFF00)>>8) == MessageBuffer[NAV_MSG_LEN-2])&&((crc&0x00FF) == MessageBuffer[NAV_MSG_LEN-1])) return 1;
    else return 0;
};


// NAV module initializer
static int __rtai_nav_init(void)
{
    RTIME tick_period;    // Determines the task's period
    RTIME now;        // current time in ns
    
    // Opens the NAV communication
    if (rt_open_nav() < 0) {
        rt_printk("Nao abriu o dispositivo NAV\n");
        return -1;
        }

    // configures the NAV communication
    /*if (rt_cfg_nav() < 0) {
        rt_printk("Nao configurou o dispositivo NAV\n");
        return -1;
        }*/

    // Creates the real time task
    if (rt_task_init(&task_nav, func_nav, 0, 5000, NAV_TASK_PRIORITY, 0, 0) < 0) {
        rt_printk("Falha ao criar a tarefa de tempo real do NAV\n");
        return -1;
    }
    
    // Determines the task execution period as being a 1 ms multiple (PERIOD* 1 ms)
    tick_period = NAV_PERIOD*start_rt_timer(nano2count(A_MILLI_SECOND));
    now = rt_get_time();
    
    //Launches the main task as a periodic task
    if (rt_task_make_periodic(&task_nav, now + tick_period, tick_period) < 0) {
        rt_printk("[__rtai_nav_init]:Nao consegui lancar tarefa de tempo real periodicamente\n");
                return -1;
    }
    return 0;
};

// NAV module's destructor
static void __rtai_nav_cleanup(void)
{
    //Terminates the real time task
    rt_task_delete(&task_nav);
    rt_close_serial(NAV_PORT);    
};

module_init(__rtai_nav_init);
module_exit(__rtai_nav_cleanup);

//Allows the other module (fdc_slave) to get the nav data
//The function returns 1 for new data and 0 for old data
int rt_get_nav_data(msg_nav_t *msg)
{
    // Fills the received msg with the global variable
    //X_AXIS, Y_AXIS, Z_AXIS are enums defined on message.h

    //Euler angles (estimated by the NAV)
    msg->angle[X_AXIS] = global_msg_nav.angle[X_AXIS]; //roll
    msg->angle[Y_AXIS] = global_msg_nav.angle[Y_AXIS]; //pitch
    msg->angle[Z_AXIS] = global_msg_nav.angle[Z_AXIS]; //heading

    //Angular Rates
    msg->gyro[X_AXIS] = global_msg_nav.gyro[X_AXIS]; //p
    msg->gyro[Y_AXIS] = global_msg_nav.gyro[Y_AXIS]; //q
    msg->gyro[Z_AXIS] = global_msg_nav.gyro[Z_AXIS]; //r

    //Accelarations
    msg->accel[X_AXIS] = global_msg_nav.accel[X_AXIS]; //x''
    msg->accel[Y_AXIS] = global_msg_nav.accel[Y_AXIS]; //y''
    msg->accel[Z_AXIS] = global_msg_nav.accel[Z_AXIS]; //z''

    //Velocities
    msg->nVel = global_msg_nav.nVel;
    msg->eVel = global_msg_nav.eVel;
    msg->dVel = global_msg_nav.dVel;

    //Longitude
    msg->longitude = global_msg_nav.longitude;

    //Latitude
    msg->latitude = global_msg_nav.latitude;

    //Altitude
    msg->altitude = global_msg_nav.altitude;
    
    //Temperature Voltage
    msg->temp = global_msg_nav.temp;

    //Internal Time
    msg->time_stamp = global_msg_nav.time_stamp;

    //Internal status mask
    msg->internal_status = global_msg_nav.internal_status;
    //Internal error mask
    msg->internal_error = global_msg_nav.internal_error;

    if (global_new_data_nav == 1) // So it is new data
    {
        // Resets the new data variable
        global_new_data_nav = 0;
        return 1; // New data
    }

    return 0; // Old data
};

