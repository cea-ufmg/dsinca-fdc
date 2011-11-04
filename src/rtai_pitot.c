/*
    rtai_nav.h - wireless pitot tube module
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

#include "rtai_pitot.h"

MODULE_AUTHOR("Victor Costa da Silva Campos");
MODULE_DESCRIPTION("Real time data acquisition of the wireless pitot tube");
MODULE_LICENSE("GPL");

/*--------------------------------------------------------------------------------------------
                    PITOT FUNCTIONS
--------------------------------------------------------------------------------------------*/

// Opens the PITOT communication
int rt_open_pitot(void) 
{

    // Opens the PITOT serial port for communication
    if (rt_open_serial(PITOT_PORT,PITOT_DEFAULT_BAUD) < 0) {
        rt_printk("Nao abriu a serial do PITOT\n");
                 return -1; // Error
        }
        rt_printk("Abriu a porta serial do PITOT\n");
        return 0;
}


// Main function executed by the PITOT real time task
void func_pitot(int t)
{
    unsigned char msgbuf[PITOT_MSG_LEN]; //buffer for receiving the message
        
    while (1) { // while the modules doesn't terminates        
        // Tries to process incoming messages
        global_msg_pitot.validade = rt_process_pitot_serial(msgbuf);
                    
        // if it is a valid message, updates the global variable
        if (global_msg_pitot.validade == 1)
        {
            //Sets the new data variable
            global_new_data_pitot = rt_convert_pitot_data(&global_msg_pitot, msgbuf);
        }
        
        //Waits for 20ms (50Hz)
        rt_task_wait_period();
    }

    return (void)0;
};

//Transform a 2-complement temperature data to a common int_16
int convert_temp_int_data(unsigned char msb,unsigned char lsb){
    unsigned short word = (msb << 8)|lsb; //bigger than needed to avoid trouble
    int data;
    if (word & 0x2000) {
        word ^= 0xFFFF; //XOR
        word++;
        data = -word;
    }
    else {
        data = word;
    }
    return data;
};


//Convert the message received (msgbuf_in) to engineering units (msg)
int rt_convert_pitot_data(msg_pitot_t* msg,unsigned char* msgbuf)
{
    int16_t aux;
    int32_t aux2;

    //Static Pressure
    aux2 = (msgbuf[0]<<16)|(msgbuf[1]<<8)|(msgbuf[2]);
    msg->static_pressure = (float)PITOT_RAW2STATIC(aux2);

    //Temperature
    aux = convert_temp_int_data(msgbuf[3],msgbuf[4]);
    msg->temperature = (float)PITOT_RAW2TEMP(aux);
    
    //Dynamic pressure
    aux = (msgbuf[5]<<8)|(msgbuf[6]);
    msg->dynamic_pressure = (float)PITOT_RAW2DYNAMIC(aux);
    
    //Angle of attack
    aux = (msgbuf[7]<<8)|(msgbuf[8]);
    msg->attack_angle = (float)PITOT_RAW2ALPHA(aux);
    
    //Sideslip
    aux = (msgbuf[9]<<8)|(msgbuf[10]);
    msg->sideslip_angle = (float)PITOT_RAW2BETA(aux);

    return 1;
};

//Gets a data packet
int rt_process_pitot_serial(unsigned char* MessageBuffer)
{
    static unsigned char state = 0;      // binary state variable (0 -> waiting for header/ 1 -> filling message)
    unsigned char ch;            // Current byte in the serial port

    static unsigned char MessageIndex = 0;            // Current message index

    while (rt_bytes_avail_serial(PITOT_PORT)) // Checks if there are data available
    {
        //Get a byte from the serial port.
        ch = rt_getch_serial(PITOT_PORT);
        ch=ch&0xFF;

        switch (state)
        {
            case 0:  //Look for the 1st header char
                if (ch == PITOT_HEADER_CHAR) state++; //we found it, so we get the rest of the message
            break;
            /////////////////////////////////////////////////////////////////////////
            case 1:  //Look for the 2nd header char
                if (ch == PITOT_HEADER_CHAR) state++; //we found it, so we get the rest of the message
                else state = 0;
            break;
            /////////////////////////////////////////////////////////////////////////
            case 2: //Fill the message buffer
                MessageBuffer[MessageIndex++] = ch; //Save the byte
                //checks to see if we completed the message
                if (MessageIndex == PITOT_MSG_LEN) {
                    MessageIndex = 0; state = 0; //resets the finite state machine
                    return 1;
                }
            break;
        };
    };
    return -1;    // No available data
};

// PITOT module initializer
static int __rtai_pitot_init(void)
{
    RTIME tick_period;    // Determines the task's period
    RTIME now;        // current time in ns
    
    // Opens the PITOT communication
    if (rt_open_pitot() < 0) {
        rt_printk("Nao abriu o dispositivo PITOT\n");
        return -1;
        }

    // Creates the real time task
    if (rt_task_init(&task_pitot, func_pitot, 0, 5000, PITOT_TASK_PRIORITY, 0, 0) < 0) {
        rt_printk("Falha ao criar a tarefa de tempo real do PITOT\n");
        return -1;
    }
    
    // Determines the task execution period as being a 1 ms multiple (PERIOD* 1 ms)
    tick_period = PITOT_PERIOD*start_rt_timer(nano2count(A_MILLI_SECOND));
    now = rt_get_time();
    
    //Launches the main task as a periodic task
    if (rt_task_make_periodic(&task_pitot, now + tick_period, tick_period) < 0) {
        rt_printk("[__rtai_pitot_init]:Nao consegui lancar tarefa de tempo real periodicamente\n");
                return -1;
    }
    return 0;
};

// PITOT module's destructor
static void __rtai_pitot_cleanup(void)
{
    //Terminates the real time task
    rt_task_delete(&task_pitot);
    rt_close_serial(PITOT_PORT);    
};

module_init(__rtai_pitot_init);
module_exit(__rtai_pitot_cleanup);

//Allows the other module (fdc_slave) to get the pitot data
//The function returns 1 for new data and 0 for old data
int rt_get_pitot_data(msg_pitot_t *msg)
{

    msg->static_pressure = global_msg_pitot.static_pressure;
    msg->temperature = global_msg_pitot.temperature;
    msg->dynamic_pressure = global_msg_pitot.dynamic_pressure;
    msg->attack_angle = global_msg_pitot.attack_angle;
    msg->sideslip_angle = global_msg_pitot.sideslip_angle;        

    if (global_new_data_pitot == 1) // So it is new data
    {
        // Resets the new data variable
        global_new_data_pitot = 0;
        return 1; // New data
    }

    return 0; // Old data
};

