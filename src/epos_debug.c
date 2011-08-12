/*
adsfsadfsdafdasfsadfdsfdsafdsafdsads	epos_debug.c - Maxxon motor EPOS real time device driver (module)
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

MODULE_AUTHOR("Dimas Abreu Dutra");
MODULE_DESCRIPTION("TODO");
MODULE_LICENSE("GPL");

/** Module parameters **/
static int action = 0;
MODULE_PARM (action, "i");
MODULE_PARM_DESC (action, "The debug action to be performed.");

static int index = 0;
MODULE_PARM (index, "i");
MODULE_PARM_DESC (index, "WriteObject index.");

static int subindex = 0;
MODULE_PARM (subindex, "i");
MODULE_PARM_DESC (subindex, "WriteObject subindex.");

static int nodeid = 0;
MODULE_PARM (nodeid, "i");
MODULE_PARM_DESC (nodeid, "WriteObject nodeid.");

static int data = 0;
MODULE_PARM (data, "i");
MODULE_PARM_DESC (data, "WriteObject data.");

void dwrite_object(u16,u8,u8,u32);
extern char *dresponse_ack;
extern char *dinbound_payload;
extern int *dinbound_payload_len;

#define CONTROL_WORD_INDEX 0x6040
#define SWITCH_ON_CMD 0x07
#define ENABLE_OPERATION_CMD 0x0F

static int __init debug_module_init() {
  switch (action) {
  case 0:
    dwrite_object(index,subindex,nodeid,data);
  case 1:
    //Switch on
    dwrite_object(CONTROL_WORD_INDEX,0,nodeid,SWITCH_ON_CMD);
    break;
  case 2:
    //Enable operation
    dwrite_object(CONTROL_WORD_INDEX,0,nodeid,ENABLE_OPERATION_CMD);
    break;
  case 3:
    //mode of operation: velocity
    dwrite_object(0x6060,0,0,0xFE);
    break;
  default:
    printk("unknown action %d\n", action);
  }
  
  return 0;
}

static void __exit debug_module_cleanup() {
  int i;
  
  printk("response ack: %c\n",*dresponse_ack);

  if (*dinbound_payload_len <= 0) return;

  printk("msg:");
  for (i=0; i<*dinbound_payload_len; i++)
    printk(" %.2hX",dinbound_payload[i]);
  printk("\n");
}

module_init(debug_module_init);
module_exit(debug_module_cleanup);

