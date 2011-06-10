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
 * It was tested with the EPOS 70/10, hardware version 6410h, firmware version 2033h.
 * The target RTAI version is 3.2, Linux kernel 2.4.
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

#include <stdint.h>

#include <rtai_serial.h>

static void errmsg(char * msg);
static void serial_callback(int rxavail,int txfree);

MODULE_AUTHOR("Dimas Abreu Dutra");
MODULE_DESCRIPTION("Real time device driver of Maxxon motor EPOS");
MODULE_LICENSE("GPL");

/** Module parameters **/
static int ser_port = 0;
MODULE_PARM (ser_port, "i");
MODULE_PARM_DESC (ser_port, "The rtai_serial serial port number. Integer value (default 0)");

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
#define SERIAL_FIFO_SIZE RT_SP_FIFO_SIZE_8

/** Return codes **/
typedef enum {
  SUCCESS,
  SERIAL_BUF_FULL,
  UNEXPECTED_ERROR
} write_status_t;

static int __init epos_init() {
  int err;
  
  err = rt_spopen(ser_port, baud,  DATABITS, STOPBITS,
		  PARITY, HARDCTRL, SERIAL_FIFO_SIZE);
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

static write_status_t send_opcode(uint8_t opcode) {
  int num_not_written = rt_spwrite(ser_port, &opcode, 1);

  switch (num_not_written) {
  case 0: return SUCCESS;
  case 1: return SERIAL_BUF_FULL;
    //The default will run if rt_spwrite returns -ENODEV.
  default: return UNEXPECTED_ERROR;
  }
}

static void serial_callback(int rxavail,int txfree) {
  printk("serial callback, rxavail: %d| txfree: %d\n",rxavail,txfree);
}

static void errmsg(char* msg){
  printk("EPOS driver: %s\n",msg);
}
