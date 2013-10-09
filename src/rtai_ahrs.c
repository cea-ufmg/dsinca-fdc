/*
    rtai_ahrs.c - AHRS400CD-200 real time data acquisition module
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

#include "rtai_ahrs.h"

MODULE_AUTHOR("Victor Costa da Silva Campos");
MODULE_DESCRIPTION("Real time data acquisition of xbow AHRS400DC-200");
MODULE_LICENSE("GPL");

/*--------------------------------------------------------------------------------------------
                    AHRS FUNCTIONS
--------------------------------------------------------------------------------------------*/

// Opens the AHRS communication
int rt_open_ahrs(void) 
{    
    // Opens the AHRS serial port for communication
    if (rt_open_serial(AHRS_PORT,AHRS_DEFAULT_BAUD) < 0) {
        rt_printk("Nao abriu a serial do AHRS\n");
                 return -1; // Error
        }
    
    rt_printk("Abriu a porta serial do AHRS com sucesso\n");
    return 0; // Success
};

//Configures the AHRS communication
int rt_cfg_ahrs(void)
{
    //Sets the AHRS to polled mode so it doesn't fill us with messages
    //while we're trying to configure it
    rt_putch_serial(AHRS_PORT,POLLED_MODE);
    rt_flush_serial(AHRS_PORT);//Flush the data out to the serial port
    //Sleeps for 100ms (waiting for it to understand it shouldn't be sending packets anymore)
    rt_busy_sleep(100000000);

    //Discards available messages
    while (rt_bytes_avail_serial(AHRS_PORT)) // Evaluates if there are availa
    {
        rt_clear_serial(AHRS_PORT);
    };

    //Tries to ping the device
    rt_putch_serial(AHRS_PORT,PING);
    rt_flush_serial(AHRS_PORT);
    //checks if we got the right answer
    rt_busy_sleep(100000000);
    int ch = rt_getch_serial(AHRS_PORT);
    ch = ch&0xFF;
    if (ch != PING_RESPONSE) {
        printk("[rt_open_ahrs]: Resposta errada para o ping! Abortando...");
        return -1; //wrong ping answer -> abort
    };
    
    //Gets the AHRS version and print it on the screen
    rt_putch_serial(AHRS_PORT,QUERY_VERSION);
    rt_flush_serial(AHRS_PORT);//Flush the data out to the serial port
    //Sleeps for 100ms (waiting for it to understand)
    rt_busy_sleep(100000000);

    //Mounts the message received
    char version_info[QUERY_VERSION_LENGTH+1];
    int i = 0;
    while ((rt_bytes_avail_serial(AHRS_PORT))&&(i < QUERY_VERSION_LENGTH)) // Evaluates if there are available bytes
    {
        //Get a byte from the serial port.
        int ch = rt_getch_serial(AHRS_PORT);
        ch=ch&0xFF;
        version_info[i] = ch;
        ++i;
    };
    version_info[QUERY_VERSION_LENGTH] = *("\0");
    //print it out
    printk("Connected to the following AHRS: %s\n",version_info);

    // Tries to put the AHRS in ANGLE MODE 10 times, if it fails, abort execution
    int count_try;
    for (count_try=0; count_try<10; ++count_try) {
        rt_putch_serial(AHRS_PORT,ANGLE_MODE); //sends an angle mode msg
        rt_flush_serial(AHRS_PORT);//Flush the data out to the serial port
        //Sleeps for 50ms (waiting for it to understand)
        rt_busy_sleep(50000000);

        //Checks wether we've got the right answer
        if (rt_bytes_avail_serial(AHRS_PORT)) // Evaluates if there are available bytes
        {
            //Get a byte from the serial port.
            int ch = rt_getch_serial(AHRS_PORT);
            ch=ch&0xFF;
            //checks wether it's the right answer
            if (ch == ANGLE_MODE_RESPONSE) break;
            else continue;
        }
        else {
        //no available byte
            continue;
        };
    };
    //checks whether or not we were succesful
    if (count_try >= 10) {
        printk("[rt_open_ahrs]: não consegui mudar o modo do AHRS");
        return -1; //couldn't set the AHRS mode
    };

    //if needed insert here the code to change the ahrs baud rate
    //though it should be noted that the default is 38400
    //and only 12000 are needed for the 50Hz (8*30*50)
    
    //CONTINUOUS MODE
    //Discards available messages - needed to avoid unnecessary trouble in the next setting
    while (rt_bytes_avail_serial(AHRS_PORT)) // Evaluates if there are available bytes
    {
        //Get a byte from the serial port.
        rt_getch_serial(AHRS_PORT);
    };
    // Tries to put the AHRS in CONTINUOUS MODE 10 times, if it fails, abort execution
    for (count_try=0; count_try<10; ++count_try) {
        rt_putch_serial(AHRS_PORT,CONTINUOUS_MODE); //sends a continuous mode msg
        rt_flush_serial(AHRS_PORT);//Flush the data out to the serial port
        //Sleeps for 10ms (waiting for it to understand)
        rt_busy_sleep(10000000);

        //Checks wether we've got any answer
        if (rt_bytes_avail_serial(AHRS_PORT)) // Evaluates if there are available bytes
        {
            break;
        }
        else {
        //no available byte
            continue;
        };
    };
    //checks whether or not we were succesful
    if (count_try >= 10) {
        printk("[rt_open_ahrs]: não consegui mudar o modo do AHRS");
        return -1; //couldn't set the AHRS mode
    }
    else {
        return 0; //success
    }; 
};

// Main function executed by the AHRS real time task
void func_ahrs(int t)
{
    unsigned char msgbuf[AHRS_MSG_LEN]; //buffer for receiving the message
    int read_status;
        
    while (1) { // while the modules doesn't terminates        
        // Tries to process incoming messages
        read_status = rt_process_ahrs_serial(msgbuf);
	
        // if it is a valid message, updates the global variable
        if (read_status != -1)
        {
            //Sets the new data variable
            global_msg_ahrs.validade = read_status;
            global_new_data = rt_convert_ahrs_data(&global_msg_ahrs, msgbuf);
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

//Convert the message received (msgbuf) to engineering units (msg)
int rt_convert_ahrs_data(msg_ahrs_t* msg,unsigned char* msgbuf)
{
    
    int16_t aux;
    uint16_t u_aux;
    
    //Euler angles (estimated by the AHRS)
    aux = convert_int_data(msgbuf[0],msgbuf[1]);
    msg->angle[0] = (float)AHRS_RAW2ANGLE(aux);
    aux = convert_int_data(msgbuf[2],msgbuf[3]);
    msg->angle[1] = (float)AHRS_RAW2ANGLE(aux);
    aux = convert_int_data(msgbuf[4],msgbuf[5]);
    msg->angle[2] = (float)AHRS_RAW2ANGLE(aux);

    //Angular Rates
    aux = convert_int_data(msgbuf[6],msgbuf[7]);
    msg->gyro[0] = (float)AHRS_RAW2RATE(aux);
    aux = convert_int_data(msgbuf[8],msgbuf[9]);
    msg->gyro[1] = (float)AHRS_RAW2RATE(aux);
    aux = convert_int_data(msgbuf[10],msgbuf[11]);
    msg->gyro[2] = (float)AHRS_RAW2RATE(aux);

    //Accelarations
    aux = convert_int_data(msgbuf[12],msgbuf[13]);
    msg->accel[0] = (float)AHRS_RAW2ACCEL(aux);
    aux = convert_int_data(msgbuf[14],msgbuf[15]);
    msg->accel[1] = (float)AHRS_RAW2ACCEL(aux);
    aux = convert_int_data(msgbuf[16],msgbuf[17]);
    msg->accel[2] = (float)AHRS_RAW2ACCEL(aux);

    //Magnetic Field
    aux = convert_int_data(msgbuf[18],msgbuf[19]);
    msg->magnet[0] = (float)AHRS_RAW2MAG(aux);
    aux = convert_int_data(msgbuf[20],msgbuf[21]);
    msg->magnet[1] = (float)AHRS_RAW2MAG(aux);
    aux = convert_int_data(msgbuf[22],msgbuf[23]);
    msg->magnet[2] = (float)AHRS_RAW2MAG(aux);

    //Temperature Voltage
    u_aux = (msgbuf[24]<<8)|msgbuf[25];
    msg->temp = (float)AHRS_RAW2TEMP(u_aux);

    //Internal Time
    u_aux = (msgbuf[26]<<8)|msgbuf[27];
    msg->time_stamp = (float)AHRS_RAW2TIME(u_aux);

    return 1;
};

//Gets a data packet
int rt_process_ahrs_serial(unsigned char* MessageBuffer)
{
    static enum {
      SEARCHING_HEADER,
      FILLING_BUFFER
    } state = SEARCHING_HEADER;
    int ch;            // Current byte in the serial port
    int checksum_status;

    //unsigned char circBuf[AHRS_MSG_LEN]; //circular buffer for receiving the data
    //static unsigned char BufferOffset = 0; //initial point on the circular buffer
    static unsigned char MessageIndex = 0;            // Current message index
    static unsigned char RecoverIndex = AHRS_MSG_LEN; // Index of the byte to be recovered

    while (rt_bytes_avail_serial(AHRS_PORT) || RecoverIndex < AHRS_MSG_LEN) // Checks if there are data available
    {
        //Get the next byte
        ch = RecoverIndex < AHRS_MSG_LEN ? MessageBuffer[RecoverIndex++] : rt_getch_serial(AHRS_PORT);
        ch=ch&0xFF;

        switch (state)
        {
            case SEARCHING_HEADER:
                if (ch == AHRS_HEADER)
                {
                    state = FILLING_BUFFER; //we found it, so we get the rest of the message
                }
            break;
            /////////////////////////////////////////////////////////////////////////
            case FILLING_BUFFER:
                MessageBuffer[MessageIndex++] = ch; //Save the byte
                //checks to see if we completed the message
                if (MessageIndex == AHRS_MSG_LEN) {
                    MessageIndex = 0; state = SEARCHING_HEADER; //resets the finite state machine
                    //checks the crc and returns 0 (failure) or 1 (success)
                    checksum_status = rt_chksum_check(MessageBuffer);
		    if (!checksum_status)
		      RecoverIndex = 0;
		    else 
		      return 1;
                }
            break;
        };
    };
    return -1;    // No available data
};

//checks the message checksum and returns 1 (correct) or 0 (wrong)
//checksum is available on the last msg byte
int rt_chksum_check(unsigned char* MessageBuffer){
    //checksum is given by the sum of all message bytes (chksum not included) divided by 256
    unsigned long sum = 0;
    int i = 0;
    for (i = 0; i < (AHRS_MSG_LEN-1); ++i) {
        sum += (MessageBuffer[i]&0xFF);
    };
    if ((sum % 256) == (MessageBuffer[AHRS_MSG_LEN-1]&0xFF))
        return 1;
    else
        return 0;
};

// AHRS module initializer
static int __rtai_ahrs_init(void)
{
    
    RTIME tick_period;    // Determines the task's period
    RTIME now;        // current time in ns
    
    // Opens the AHRS communication
    if (rt_open_ahrs() < 0) {
        rt_printk("Nao abriu o dispositivo AHRS\n");
        return -1;
        }

    //Configures the device
    if (rt_cfg_ahrs() < 0) {
        rt_printk("Nao configurou o dispositivo AHRS\n");
        return -1;
    }

    // Creates the real time task
    if (rt_task_init(&task_ahrs, func_ahrs, 0, 5000, AHRS_TASK_PRIORITY, 0, 0) < 0) {
        rt_printk("Falha ao criar a tarefa de tempo real do AHRS\n");
        return -1;
    }
    
    // Determines the task execution period as being a 1 ms multiple (PERIOD* 1 ms)
    tick_period = AHRS_PERIOD*start_rt_timer(nano2count(A_MILLI_SECOND));
    now = rt_get_time();
    
    //Launches the main task as a periodic task
    if (rt_task_make_periodic(&task_ahrs, now + tick_period, tick_period) < 0) {
        rt_printk("Nao consegui lancar tarefa de tempo real periodicamente\n");
                return -1;
    }
    return 0;
};

// AHRS module's destructor
static void __rtai_ahrs_cleanup(void)
{

    //Terminates the real time task
    rt_task_delete(&task_ahrs);
    if (rt_clear_serial(AHRS_PORT) == 0)
        rt_printk("Limpou a serial do AHRS com sucesso\n");
    else
        rt_printk("Não conseguiu limpar a porta serial do AHRS\n");
    if (rt_close_serial(AHRS_PORT) == 0)
        rt_printk("Fechou a parta serial do AHRS com sucesso\n");
    else
        rt_printk("Não conseguiu fechar a porta serial do AHRS\n");    

};

module_init(__rtai_ahrs_init);
module_exit(__rtai_ahrs_cleanup);

//Allows the other module (fdc_slave) to get the ahrs data
//The function returns 1 for new data and 0 for old data
int rt_get_ahrs_data(msg_ahrs_t *msg)
{
    // Fills the received msg with the global variable
    //X_AXIS, Y_AXIS, Z_AXIS are enums defined on message.h

    //Euler angles (estimated by the AHRS)
    msg->angle[X_AXIS] = global_msg_ahrs.angle[X_AXIS]; //roll
    msg->angle[Y_AXIS] = global_msg_ahrs.angle[Y_AXIS]; //pitch
    msg->angle[Z_AXIS] = global_msg_ahrs.angle[Z_AXIS]; //heading

    //Angular Rates
    msg->gyro[X_AXIS] = global_msg_ahrs.gyro[X_AXIS]; //p
    msg->gyro[Y_AXIS] = global_msg_ahrs.gyro[Y_AXIS]; //q
    msg->gyro[Z_AXIS] = global_msg_ahrs.gyro[Z_AXIS]; //r

    //Accelarations
    msg->accel[X_AXIS] = global_msg_ahrs.accel[X_AXIS]; //x''
    msg->accel[Y_AXIS] = global_msg_ahrs.accel[Y_AXIS]; //y''
    msg->accel[Z_AXIS] = global_msg_ahrs.accel[Z_AXIS]; //z''

    //Magnetic Field
    msg->magnet[X_AXIS] = global_msg_ahrs.magnet[X_AXIS]; //Bx
    msg->magnet[Y_AXIS] = global_msg_ahrs.magnet[Y_AXIS]; //By
    msg->magnet[Z_AXIS] = global_msg_ahrs.magnet[Z_AXIS]; //Bz

    //Temperature Voltage
    msg->temp = global_msg_ahrs.temp;

    //Internal Time
    msg->time_stamp = global_msg_ahrs.time_stamp;

    //Validade
    msg->validade = global_msg_ahrs.validade;
    
    if (global_new_data == 1) // So it is new data
    {
        // Resets the new data variable
        global_new_data = 0;
        return 1; // New data
    }

    return 0; // Old data
};

