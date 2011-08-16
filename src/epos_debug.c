/*
        epos_debug.c - Module to test and debug the EPOS real-time driver.
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

/**
 * @brief TODO
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

extern char *dresponse_ack;
extern char *dinbound_payload;
extern int *dinbound_payload_len;
extern char *doutbound_payload;
extern int *doutbound_payload_len;

#define CONTROL_WORD_INDEX 0x6040
#define FAULT_RESET_CMD 0x80
#define SHUTDOWN_CMD 0x06
#define SWITCH_ON_CMD 0x07
#define ENABLE_OPERATION_CMD 0x0F

#define MODES_OPERATION_INDEX 0x6060
#define HOMING_MODE 0x06
#define PROFILE_VELOCITY_MODE 0x03
#define PROFILE_POSITION_MODE 0x01
#define POSITION_MODE 0xFF
#define VELOCITY_MODE 0xFE
#define CURRENT_MODE 0xFD
#define DIAGNOSTIC_MODE 0xFC
#define MASTER_ENCODER_MODE 0xFB
#define STEP_DIRECTION_MODE 0xFA

#define VELOCITY_MODE_SP_INDEX 0x206B
#define DEFAULT_VELOCITY_SP 1000

static int __init debug_module_init() {
  switch (action) {
  case 0:
    epos_write_object(index,subindex,nodeid,data);
    break;
  case -1:
    epos_read_object(index,subindex,nodeid);
    break;
  case 1:
    //Fault reset
    epos_write_object(CONTROL_WORD_INDEX,0,nodeid,FAULT_RESET_CMD);
    break;
  case 2:
    //Shutdown
    epos_write_object(CONTROL_WORD_INDEX,0,nodeid,SHUTDOWN_CMD);
    break;
  case 3:
    //Switch on
    epos_write_object(CONTROL_WORD_INDEX,0,nodeid,SWITCH_ON_CMD);
    break;
  case 4:
    //Enable operation
    epos_write_object(CONTROL_WORD_INDEX,0,nodeid,ENABLE_OPERATION_CMD);
    break;
  case 5:
    //mode of operation: velocity
    epos_write_object(MODES_OPERATION_INDEX,0,0,VELOCITY_MODE);
    break;
  case 6:
    //mode of operation: velocity
    epos_write_object(VELOCITY_MODE_SP_INDEX,0,0,DEFAULT_VELOCITY_SP);
    break;
  default:
    printk("unknown action %d\n", action);
  }
  
  return 0;
}

static void __exit debug_module_cleanup() {
  int i;
  
  printk("response ack: %c\n",*dresponse_ack);

  if (*doutbound_payload_len > 0) {
    printk("sent msg:");
    for (i=0; i<*doutbound_payload_len; i++)
      printk(" %.2hX",(u8)doutbound_payload[i]);
    printk("\n");
  }
  
  if (*dinbound_payload_len > 0) {
    printk("recv msg:");
    for (i=0; i<*dinbound_payload_len; i++)
      printk(" %.2hX",(u8)dinbound_payload[i]);
    printk("\n");
  }
}

module_init(debug_module_init);
module_exit(debug_module_cleanup);

