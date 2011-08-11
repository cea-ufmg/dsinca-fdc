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

//OPCODES
enum {
  OPCODE_RESPONSE = 0x00
};
  
/** Driver state machine codes **/
typedef enum {
  READY,
  SENDING_OPCODE,
  WAITING_BEGIN_ACK,
  SENDING_DATA,
  WAITING_END_ACK,
  WAITING_RESPONSE_OPCODE,
  SENDING_RESPONSE_BEGIN_ACK,
  WAITING_RESPONSE_DATA,
  SENDING_RESPONSE_END_ACK
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

static char response_ack;

/** Declare the internal functions **/
static write_status_t send_opcode(char opcode);
static void read_begin_ack();
static void read_end_ack();
static void send_payload();
static void read_response_opcode();
static void read_response_payload();
static u16 crc_byte(u16,u8);
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
  switch (state) {
  case READY: errmsg("READY"); return;
  case SENDING_OPCODE:
    if (txfree == MAX_PAYLOAD) state = WAITING_BEGIN_ACK; //opcode was sent    
    //fall-through for the case the end of transmission and response reception
    //are handled by the same callback
  case WAITING_BEGIN_ACK:
    if (rxavail >= 1) read_begin_ack();
    break;
  case SENDING_DATA:
    if (txfree == MAX_PAYLOAD) state = WAITING_END_ACK; //data was sent
    //intentional fall-through again, same reasons
  case WAITING_END_ACK:
    if (rxavail >= 1) read_end_ack();
    break;
  case WAITING_RESPONSE_OPCODE:
    if (rxavail >= 1) read_response_opcode();
    break;
  case SENDING_RESPONSE_BEGIN_ACK:
    if (txfree == MAX_PAYLOAD)
      state = response_ack == 'O' ? WAITING_RESPONSE_DATA : READY;
  case WAITING_RESPONSE_DATA:
    if (rxavail >= inbound_payload_len) read_response_payload();
    break;
  case SENDING_RESPONSE_END_ACK:
    if (txfree == MAX_PAYLOAD) state = READY;
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
  
  if (ack == 'O' && inbound_payload_len > 0) //Okay
    state = WAITING_RESPONSE_OPCODE;
  else
    state = READY;
}

static void send_payload() {
  int num_not_written;
  
  rt_spset_thrs(ser_port, 1, outbound_payload_len);
  num_not_written = rt_spwrite(ser_port, outbound_payload,
			       -outbound_payload_len);
  if (num_not_written != 0) state = READY;
  else state = SENDING_DATA;
}

static void read_response_opcode() {
  int num_not_written;
  char opcode;
  
  rt_spread(ser_port, &opcode, 1);
  response_ack = opcode == OPCODE_RESPONSE ? 'O' : 'F';
  
  num_not_written = rt_spwrite(ser_port, &response_ack, 1);
  if (num_not_written == 1) state = READY;
  else state = SENDING_RESPONSE_BEGIN_ACK;
}

static void read_response_payload() {
  int num_not_written;
  u16 crc;
  union {
    u16 word;
    u8 bytes[2];
  } recv_crc;
  
  rt_spread(ser_port, inbound_payload, inbound_payload_len);
  memcpy(recv_crc.bytes, inbound_payload + inbound_payload_len - 2, 2);
  
  crc = crc_byte(0, inbound_payload[0]);
  crc = crc_data(crc, inbound_payload + 1, inbound_payload_len - 3);
  response_ack = crc == __le16_to_cpu(recv_crc.word) ? 'O' : 'F';
  
  num_not_written = rt_spwrite(ser_port, &response_ack, 1);
  if (num_not_written == 1) state = READY;
  else state = SENDING_RESPONSE_END_ACK;
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
  crc.word = crc_byte(0, opcode);
  crc.word = crc_byte(crc.word, len_m1);
  crc.word = __cpu_to_le16(crc_data(crc.word, outbound_payload+1, 8));

  //Append the CRC to the end of the packet
  memcpy(outbound_payload + 9, crc.bytes, 2);

  outbound_payload_len = 11;
  inbound_payload_len = 7;
  
  return SUCCESS;
}

static void debug() {
  write_status_t st = write_object(0x6040,0,0,0xF);//Enable operation
  //write_status_t st = write_object(0x6060,0,0,0xFE);//mode of oper: velocity
}

/**
 * The code below calculates the CRC of the message as expected by the EPOS. In
 * the manual they said the crc-ccitt algorithm is used but the code and
 * examples provided show that actually the XMODEM variety is used so the kermit
 * algorithm (for which there is a standard kernel module) cannot be used.
 */

static u16 crc_byte(u16 crc, u8 data) {
  int i;
  crc ^= (u16)data << 8;
  
  for (i = 0; i < 8; i++)
    if (crc & 0x8000)
      crc = (crc << 1) ^ 0x1021;
    else
      crc <<= 1;
  
  return crc;
}

static u16 crc_data(u16 crc, u8* data, int len) {
  u8 low_byte, high_byte;
  
  len -= (len % 2); //For safety.
  
  while (len > 0) {
    low_byte  = *data++;
    high_byte = *data++;

    crc = crc_byte(crc, high_byte);
    crc = crc_byte(crc, low_byte);
    
    len -= 2;
  }

  return crc;
}
