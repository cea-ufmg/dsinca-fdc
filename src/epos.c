/*
        epos.c - Maxon motor EPOS real time device driver (module)
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
 * This is the Maxon motor EPOS real time device driver for RTAI linux.
 * It was tested with the EPOS 70/10, hardware version 6410h, firmware version
 * 2033h. The target RTAI version is 3.2, Linux kernel 2.4.
 *
 * Some general info on the code: the rtai_serial module can return -ENODEV in
 * in pretty much every function it has. That should only happen if the serial
 * we are requesting does not exist (was not registered) in the driver. Since
 * our module opens the serial device in the module_init it will only treat
 * this error during initialization, and will not initialize if the port number
 * is invalid. If any -ENODEV is encountered from a serial port function it will
 * be returned upwards, but it theoretically should never happen.
 *
 * In the module code we refer as payload to everything in the message after
 * the opcode. That includes the length and crc fields. Data is the stuff from
 * after the len until before the crc.
 */

#include "epos.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/timer.h>

#include <rtai_sem.h>
#include <rtai_serial.h>

// The file below defines the default serial port for our application
#include <rtai_rt_serial.h>


MODULE_AUTHOR("Dimas Abreu Dutra");
MODULE_DESCRIPTION("Real time device driver of Maxon motor EPOS");
MODULE_LICENSE("GPL");

/** Module parameters **/
static int ser_port = EPOS_PORT;
MODULE_PARM (ser_port, "i");
MODULE_PARM_DESC (ser_port, "The rtai_serial serial port number. "
          "Integer value (default 0)");

static int baud = 38400;
MODULE_PARM (baud, "i");
MODULE_PARM_DESC (baud, "The baud rate. Integer value (default 38400)");

static int timeout = 500;
MODULE_PARM (timeout, "i");
MODULE_PARM_DESC (timeout, "The communication timeout, in miliseconds. "
          "Integer value (default 500)");

/** Module definitions **/
//Serial communication parameters
#define STARTBITS 1
#define DATABITS 8
#define PARITY RT_SP_PARITY_NONE
#define STOPBITS 1
#define HARDCTRL RT_SP_NO_HAND_SHAKE
#define FIFOTRIG RT_SP_FIFO_SIZE_1

//Packet parameters
#define MAX_PAYLOAD 40 //This should not exceed SERIAL_FIFO_SIZE

//OPCODES
enum {
  OPCODE_RESPONSE = 0x00,
  OPCODE_READ_OBJECT = 0x10,
  OPCODE_WRITE_OBJECT = 0x11
};

//Semaphore state
#define INVALID_SEMAPHORE 0xFFFF

/** Driver state machine codes **/
typedef enum {
  READY,
  SENDING_OPCODE,
  WAITING_BEGIN_ACK,
  SENDING_PAYLOAD,
  WAITING_END_ACK,
  WAITING_RESPONSE_OPCODE,
  SENDING_RESPONSE_BEGIN_ACK,
  WAITING_RESPONSE_PAYLOAD,
  SENDING_RESPONSE_END_ACK
} driver_state_t;

/** Driver variables **/
epos_response_status_t epos_response_status = EPOS_RESPONSE_NONE;
int epos_num_response_words = 0;

static driver_state_t state;

static int tx_buffer_size;
static char outbound_payload[MAX_PAYLOAD];
static char inbound_payload[MAX_PAYLOAD];
static int outbound_payload_len;
static int inbound_payload_len;
static char response_ack;

static SEM mutex;
struct timer_list timeout_timer;

/** Declare the internal functions **/
static int send_opcode(char opcode);
static void read_begin_ack();
static void read_end_ack();
static void send_payload();
static void read_response_opcode();
static void read_response_payload();
static void timeout_function(unsigned long);
static void comm_error();
static void comm_done();
static void set_outbound_len_crc(u8, u8);
static void set_outdata_word(int,u16);
static void set_outdata_dword(int,u32);
static void set_outdata_bytes(int,u8,u8);
static void errmsg(char * msg);
static void serial_callback(int rxavail,int txfree);
static u16 crc_byte(u16,u8);
static u16 crc_data(u16,u8*,int);

/** Some useful macros and inline functions **/
#define TRANSMISSION_DONE(txfree) (txfree >= tx_buffer_size)
static inline void fire_timeout() {
  timeout_timer.expires = jiffies + HZ*timeout/1000;
  add_timer(&timeout_timer);
}

static inline void move_timeout() {
  mod_timer(&timeout_timer, jiffies + HZ*timeout/1000);
}

/** Module code **/
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

  tx_buffer_size = rt_spget_txfrbs(ser_port);

  //Initialize data structures
  rt_sem_init(&mutex, 1);

  init_timer(&timeout_timer);
  timeout_timer.function = &timeout_function;

  return 0;

 spset_callback_fail: rt_spclose(ser_port);
 spopen_fail: return err;
}

static void __exit epos_cleanup() {
  int err;

  //Acquire mutex
  rt_sem_wait(&mutex);
  
  err = rt_spclose(ser_port);
  if (err == -ENODEV)
    errmsg("Error closing serial: rtai_serial claims port does not exist.");

  //Invalidate mutex
  rt_sem_delete(&mutex);
}

module_init(epos_init);
module_exit(epos_cleanup);

static void serial_callback(int rxavail, int txfree) {
  switch (state) {
  case READY: return;
  case SENDING_OPCODE:
    if (TRANSMISSION_DONE(txfree)) //opcode was sent
      state = WAITING_BEGIN_ACK; 
    //fall-through for the case the end of transmission and response reception
    //are handled by the same callback
  case WAITING_BEGIN_ACK:
    if (rxavail >= 1) read_begin_ack();
    break;
  case SENDING_PAYLOAD:
    if (TRANSMISSION_DONE(txfree)) state = WAITING_END_ACK; //data was sent
    //intentional fall-through again, same reasons
  case WAITING_END_ACK:
    if (rxavail >= 1) read_end_ack();
    break;
  case WAITING_RESPONSE_OPCODE:
    if (rxavail >= 1) read_response_opcode();
    break;
  case SENDING_RESPONSE_BEGIN_ACK:
    if (TRANSMISSION_DONE(txfree)) {
      if (response_ack == 'O')
    state = WAITING_RESPONSE_PAYLOAD;
      else comm_error();
    }
    //intentional fall-through again, same reasons
  case WAITING_RESPONSE_PAYLOAD:
    if (rxavail >= inbound_payload_len) read_response_payload();
    break;
  case SENDING_RESPONSE_END_ACK:
    if (TRANSMISSION_DONE(txfree)) comm_done();
    break;    
  }
}

/**
 * @brief Sends message opcode to the EPOS.
 * @returns 0 if success, -ENOBUFS if send buffer is full, -ENODEV if serial
 * port is invalid.
 */
static int send_opcode(char opcode) {
  rt_spclear_rx(ser_port);
  rt_spclear_tx(ser_port);
  
  int num_not_written = rt_spwrite(ser_port, &opcode, 1);

  switch (num_not_written) {
  case 0:
    state = SENDING_OPCODE;
    fire_timeout();
    return 0;
  case 1: return -ENOBUFS;
  case -ENODEV: return -ENODEV;
  default: return -1; //This should never occur.
  }
}

static void read_begin_ack() {
  char ack;
  rt_spread(ser_port, &ack, 1);
  
  if (ack == 'O') //Okay
    send_payload();
  else //Fail
    comm_error();
}

static void read_end_ack() {
  char ack;
  rt_spread(ser_port, &ack, 1);
  
  if (ack == 'O' && inbound_payload_len > 0) {//Okay
    move_timeout();
    state = WAITING_RESPONSE_OPCODE;
  } else comm_error();
}

static void send_payload() {
  int num_not_written;

  move_timeout();
  
  rt_spset_thrs(ser_port, 1, outbound_payload_len);
  num_not_written = rt_spwrite(ser_port, outbound_payload,
                   -outbound_payload_len);
  if (num_not_written != 0) comm_error();
  else state = SENDING_PAYLOAD;
}

static void read_response_opcode() {
  int num_not_written;
  char opcode;

  rt_spread(ser_port, &opcode, 1);
  response_ack = opcode == OPCODE_RESPONSE ? 'O' : 'F';
  
  num_not_written = rt_spwrite(ser_port, &response_ack, 1);
  if (num_not_written == 1) comm_error();
  else {
    move_timeout();
    state = SENDING_RESPONSE_BEGIN_ACK;
  }
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
  if (num_not_written == 1) comm_error();
  else {
    move_timeout();
    state = SENDING_RESPONSE_END_ACK;
  }
}

static void timeout_function(unsigned long unused) {
  errmsg("Communication timeout.");
  state = READY;
  epos_response_status = EPOS_RESPONSE_ERROR;
}

static void comm_error() {
  del_timer(&timeout_timer);
  state = READY;
  epos_response_status = EPOS_RESPONSE_ERROR;
}

static void comm_done() {
  del_timer(&timeout_timer);
  state = READY;
  epos_response_status = EPOS_RESPONSE_SUCCESS;
}

int epos_write_object(u16 index, u8 subindex, u8 nodeid, u32 data) {
  int stat;
  int sem_count;

  //Acquire mutex
  sem_count = rt_sem_wait_if(&mutex);
  if (sem_count <= 0 || sem_count == INVALID_SEMAPHORE)
    return -EBUSY;
  
  if (state != READY) {
    rt_sem_signal(&mutex); //Release mutex
    return -EBUSY;
  }
  
  //Fill out the outbound payload
  set_outdata_word (0, index);
  set_outdata_bytes(1, subindex, nodeid);
  set_outdata_dword(2, data);
  set_outbound_len_crc(OPCODE_WRITE_OBJECT, 4);

  //Send the opcode
  stat = send_opcode(OPCODE_WRITE_OBJECT);
  if (stat != 0) {
    rt_sem_signal(&mutex); //Release mutex
    return stat;
  }
  
  //Define the answer length
  epos_num_response_words = 2;
  inbound_payload_len = epos_num_response_words*2 + 3;
  epos_response_status = EPOS_RESPONSE_WAITING;
  
  //Release mutex
  rt_sem_signal(&mutex);
  
  return 0;
}

int epos_read_object(u16 index, u8 subindex, u8 nodeid) {
  int stat;
  int sem_count;

  //Acquire mutex
  sem_count = rt_sem_wait_if(&mutex);
  if (sem_count <= 0 || sem_count == INVALID_SEMAPHORE) return -EBUSY;
  
  if (state != READY) {
    rt_sem_signal(&mutex); //Release mutex
    return -EBUSY;
  }
    
  //Fill out the outbound payload
  set_outdata_word (0, index);
  set_outdata_bytes(1, subindex, nodeid);
  set_outbound_len_crc(OPCODE_READ_OBJECT, 2);

  //Send the opcode
  stat = send_opcode(OPCODE_READ_OBJECT);
  if (stat != 0) {
    rt_sem_signal(&mutex); //Release mutex
    return stat;
  }
  
  //Define the answer length
  epos_num_response_words = 4;
  inbound_payload_len = epos_num_response_words*2 + 3;
  epos_response_status = EPOS_RESPONSE_WAITING;
  
  //Release mutex
  rt_sem_signal(&mutex);
  
  return 0;
}

epos_response_status_t read_object_response(u32 *error, u32 *data) {
  if (epos_response_status != EPOS_RESPONSE_SUCCESS)
    return epos_response_status;

  *error = epos_read_indata_dword(0);
  *data = epos_read_indata_dword(2);
  
  return EPOS_RESPONSE_SUCCESS;
}

static void set_outbound_len_crc(u8 opcode, u8 data_word_length) {
  u16 crc;
  u8 len_m1 = data_word_length - 1;
  
  outbound_payload_len = data_word_length*2 + 3;
  outbound_payload[0] = len_m1;

  crc = crc_byte(0, opcode);
  crc = crc_byte(crc, len_m1);
  crc = __cpu_to_le16(crc_data(crc, outbound_payload+1, data_word_length*2));

  set_outdata_word(data_word_length, crc);
}

/**
 * @brief Sets outbound message data word.
 *
 * Input is set on the appropriate field of the outbound_message_payload 
 * variable.
 *
 * @param windex Index of the data word to set.
 * @param value  Value the word is set to.
 */
static void set_outdata_word(int windex, u16 data) {
  union {
    u16 word;
    u8 bytes[2];
  } data_union = {__cpu_to_le16(data)};
  int payload_index = windex*2 + 1;
  
  memcpy(outbound_payload + payload_index, data_union.bytes, 2);
}

/**
 * @brief Sets outbound message data double word.
 *
 * Input is set on the appropriate field of the outbound_message_payload 
 * variable.
 *
 * @param windex Index of the data word to set.
 * @param value  Value the double word is set to.
 */
static void set_outdata_dword(int windex, u32 data) {
  union {
    u32 dword;
    u8 bytes[4];
  } data_union = {__cpu_to_le32(data)};
  int payload_index = windex*2 + 1;
  
  memcpy(outbound_payload + payload_index, data_union.bytes, 4);
}

/**
 * @brief Sets high and low bytes of an outbound message data word.
 *
 * Input is set on the appropriate field of the outbound_message_payload 
 * variable.
 *
 * @param windex    Index of the data word to set.
 * @param low_byte  Value the low byte of the word will be set to.
 * @param high_byte Value the high byte of the word will be set to.
 */
static void set_outdata_bytes(int windex, u8 low_byte, u8 high_byte) {
  int payload_index = windex*2 + 1;
  
  outbound_payload[payload_index    ] = low_byte;
  outbound_payload[payload_index + 1] = high_byte;
}

void epos_read_indata_bytes(int windex, u8 *high_byte, u8 *low_byte) {
  int payload_index = windex*2 + 1;
  
  *low_byte  = inbound_payload[payload_index];
  *high_byte = inbound_payload[payload_index + 1];
}

u16 epos_read_indata_word(int windex) {
  union {
    u16 word;
    u8 bytes[2];
  } ret;
  int payload_index = windex*2 + 1;

  memcpy(ret.bytes, inbound_payload + payload_index, 2);
  return __le16_to_cpu(ret.word);
}

u32 epos_read_indata_dword(int windex) {
  union {
    u32 dword;
    u8 bytes[4];
  } ret;
  int payload_index = windex*2 + 1;

  memcpy(ret.bytes, inbound_payload + payload_index, 4);
  return __le32_to_cpu(ret.dword);
}

/*
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

static void errmsg(char* msg){
  printk("EPOS driver: %s\n",msg);
}
