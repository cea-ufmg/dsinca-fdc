/*
	epos.c - Maxxon motor EPOS real time device driver (module)
    Copyright (C) 2011  Dimas Abreu Dutra - dimasadutra@gmail.com

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

/* 
 * This is the Maxxon motor EPOS real time device driver for RTAI linux.
 * It was tested with the EPOS 70/10, hardware version 6410h, firmware version
 * 2033h. The target RTAI version is 3.2, Linux kernel 2.4.
 *
 * Some general info on the code: the rtai_serial module can return ENODEV in
 * in pretty much every function it has. That should only happen if the serial
 * we are requesting does not exist (was not registered) in the driver. Since
 * our module opens the serial device in the module_init it will only treat
 * this error during initialization. Since the module will not initialize if
 * the serial port number is invalid, that error will be treated in the other
 * functions as an "unexpected error".
 */

#include "epos.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <rtai_serial.h>

static void errmsg(char * msg);
static void serial_callback(int rxavail,int txfree);

MODULE_AUTHOR("Dimas Abreu Dutra");
MODULE_DESCRIPTION("Real time device driver of Maxxon motor EPOS");
MODULE_LICENSE("GPL");

/** Module parameters **/
static int ser_port = 0;
MODULE_PARM (ser_port, "i");
MODULE_PARM_DESC (ser_port, "The rtai_serial serial port number. "
		  "Integer value (default 0)");

static int baud = 38400;
MODULE_PARM (baud, "i");
MODULE_PARM_DESC (baud, "The baud rate. Integer value (default 38400)");

/** Module definitions **/
//Serial communication parameters
#define STARTBITS 1
#define DATABITS 8
#define PARITY RT_SP_PARITY_NONE
#define STOPBITS 1
#define HARDCTRL RT_SP_NO_HAND_SHAKE
#define FIFOTRIG RT_SP_FIFO_SIZE_1

//Packet parameters
#define MAX_PAYLOAD 512 //This should not exceed SERIAL_FIFO_SIZE

/** Driver state machine codes **/
typedef enum {
  READY,
  SENDING_OPCODE,
  WAITING_BEGIN_ACK,
  SENDING_DATA,
  WAITING_END_ACK,
  WAITING_RESPONSE_LEN,
  WAITING_RESPONSE_DATA,
  SENDING_RESPONSE_ACK
} driver_state_t;

/** Return codes **/
typedef enum {
  SUCCESS,
  SERIAL_BUF_FULL,
  DRIVER_BUSY,
  UNEXPECTED_ERROR
} write_status_t;

/** Driver variables **/
static driver_state_t state;
static char outbound_payload[MAX_PAYLOAD];
static char inbound_payload[MAX_PAYLOAD];

static int outbound_payload_len;
static int inbound_payload_len;

static int has_response;

/** Declare the internal functions **/
static write_status_t send_opcode(char opcode);
static void read_begin_ack();
static void read_end_ack();
static void send_payload();
static void recv_response();
static u16 crc_word(u16,u16);
static u16 crc_data(u16,u8*,int);

static void debug();

static int __init epos_init() {
  int err;
  
  err = rt_spopen(ser_port, baud,  DATABITS, STOPBITS,
		  PARITY, HARDCTRL, FIFOTRIG);
  switch (err) {
  case -ENODEV:
    errmsg("Serial port number rejected by rtai_serial.");
    goto spopen_fail;
  case -EINVAL:
    errmsg("Invalid parameters for opening serial port.");
    goto spopen_fail;
  case -EADDRINUSE:
    errmsg("Serial port already in use.");
    goto spopen_fail;
  }

  err = rt_spset_callback_fun(ser_port, &serial_callback, 1, 1);
  switch (err) {
  case -EINVAL:
    errmsg("Invalid parameters for setting serial port callback.");
    goto spset_callback_fail;
  }

  debug();
  
  return 0;

 spset_callback_fail: rt_spclose(ser_port);
 spopen_fail: return err;
}

static void __exit epos_cleanup() {
  int err;
  
  err = rt_spclose(ser_port);
  if (err == -ENODEV)
    errmsg("Error closing serial: rtai_serial claims port does not exist.");
}

module_init(epos_init);
module_exit(epos_cleanup);

static void serial_callback(int rxavail, int txfree) {
  printk("serial_callback(%d,%d)\n",rxavail,txfree);
  
  switch (state) {
  case READY: errmsg("READY"); return;
  case SENDING_OPCODE:
    errmsg("s opcode");
    if (txfree == MAX_PAYLOAD) state = WAITING_BEGIN_ACK; //opcode was sent    
    //fall-through for the case the end of transmission and response reception
    //are handled by the same callback
  case WAITING_BEGIN_ACK:
    errmsg("wba");
    if (rxavail >= 1) read_begin_ack();
    break;
  case SENDING_DATA:
    errmsg("sd");
    if (txfree == MAX_PAYLOAD) state = WAITING_END_ACK; //data was sent
    //intentional fall-through again, same reasons
  case WAITING_END_ACK:
    errmsg("wea");
    if (rxavail == 1) read_end_ack();
    break;
  case WAITING_RESPONSE_LEN:
    break;
  case WAITING_RESPONSE_DATA:
    break;
  case SENDING_RESPONSE_ACK:
    break;    
  }
}

static void errmsg(char* msg){
  printk("EPOS driver: %s\n",msg);
}

static write_status_t send_opcode(char opcode) {
  int num_not_written = rt_spwrite(ser_port, &opcode, 1);

  switch (num_not_written) {
  case 0:
    state = SENDING_OPCODE;
    return SUCCESS;
  case 1: return SERIAL_BUF_FULL;
    //The default will run if rt_spwrite returns -ENODEV.
  default: return UNEXPECTED_ERROR;
  }
}

static void read_begin_ack() {
  char ack;
  rt_spread(ser_port, &ack, 1);
  
  if (ack == 'O') //Okay
    send_payload();
  else //Fail
    state = READY;
}

static void read_end_ack() {
  char ack;
  rt_spread(ser_port, &ack, 1);
  
  if (ack == 'O' && has_response) //Okay
    recv_response();
  else
    state = READY;

  printk("EPOS end ack: %c\n",ack);
}

static void send_payload() {
  int num_not_written;
  
  rt_spset_thrs(ser_port, 1, outbound_payload_len);
  num_not_written = rt_spwrite_timed(ser_port, outbound_payload,
				     outbound_payload_len,DELAY_FOREVER);
  if (num_not_written != 0) state = READY;
  else state = SENDING_DATA;
}

static void recv_response() {
}

static write_status_t write_object(u16 index,u8 subindex,
				   u8 nodeid, u32 data) {
  union {
    u16 word;
    u8 bytes[2];
  } index_union = {__cpu_to_le16(index)};
  union {
    u32 dword;
    u8 bytes[4];
  } data_union = {__cpu_to_le32(data)};
  u8 opcode = 0x11;
  u8 len_m1 = 3;//data length - 1
  union {
    u16 word;
    u8 bytes[2];
  } crc;
  write_status_t stat;
  
  if (state != READY) return DRIVER_BUSY;

  stat = send_opcode(opcode);
  if (stat != SUCCESS) return stat;

  //Fill out the payload
  outbound_payload[0] = len_m1;
  memcpy(outbound_payload +1, index_union.bytes, 2);
  outbound_payload[3] = subindex;
  outbound_payload[4] = nodeid;
  memcpy(outbound_payload + 5, data_union.bytes, 4);

  //Calculate the CRC
  crc.word = crc_word(0, ((u16)opcode << 8) + (u16)len_m1);
  crc.word = __cpu_to_le16(crc_data(crc.word, outbound_payload+1, 8));

  //Append the CRC to the end of the packet
  memcpy(outbound_payload + 9, crc.bytes, 2);

  outbound_payload_len = 11;
  has_response = 0;
  
  return SUCCESS;
}

static void debug() {
  write_status_t st = write_object(0x6040,0,0,0xF);//Enable operation
  //write_status_t st = write_object(0x6060,0,0,0xFE);//mode of operation: velocity
}

/**
 * The code below calculates the CRC of the message as expected by the EPOS. In
 * the manual they said the crc-ccitt algorithm is used but the code and
 * examples provided show that actually the XMODEM variety is used so the kermit
 * algorithm (for which there is a standard kernel module) cannot be used.
 */

/**
 * This function is adapted from the epos communication guide manual.
 */
static u16 crc_word(u16 crc, u16 data) {
  u16 carry;
  u16 shifter = 0x8000; //Initialize bitX to bit15
  
  do {
    carry = crc & 0x8000;      //Check if bit15 of CRC is set
    crc <<= 1;                 //crc = crc * 2
    if(data & shifter) crc++;  //crc = crc + 1, if bitX is set in the data
    if(carry) crc ^= 0x1021;   //crc = crc xor G(x), if carry is true
    shifter >>= 1;             //Set bitX to next lower bit (divide by 2)
  } while (shifter);
  
  return crc;
}

static u16 crc_data(u16 crc, u8* data, int len) {
  u16 curr_word;
  
  len -= (len % 2); //For safety.
  while (len > 0) {
    curr_word = *data++;
    curr_word += ((u16) *data++) << 8;

    crc = crc_word(crc, curr_word);
    
    len -= 2;
  }

  return crc;
}
