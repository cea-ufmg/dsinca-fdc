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
    epos_fault_reset(nodeid);
    break;
  case 2:
    //Shutdown
    epos_shutdown(nodeid);
    break;
  case 3:
    //Switch on
    epos_switch_on(nodeid);
    break;
  case 4:
    //Enable operation
    epos_enable_operation(nodeid);
    break;
  case 5:
    //mode of operation: velocity
    epos_set_mode(nodeid, EPOS_VELOCITY_MODE);
    break;
  case 6:
    //Write default velocity set-point
    epos_write_object(EPOS_VELOCITY_MODE_SP_INDEX, 0,
		      nodeid, DEFAULT_VELOCITY_SP);
    break;
  default:
    printk("unknown action %d\n", action);
  }
  
  return 0;
}

static void __exit debug_module_cleanup() {
  int i;

  switch (epos_response_status) {
  case EPOS_RESPONSE_SUCCESS: break;
  case EPOS_RESPONSE_NONE:
    printk("Request has no response.\n");
    return;
  case EPOS_RESPONSE_WAITING:
    printk("Still waiting response...\n");
    return;    
  case EPOS_RESPONSE_ERROR:
    printk("Response error!!!\n");
    return;    
  }
  
  printk("received response data:");
  for (i=0; i<epos_num_response_words; i++)
    printk(" %.4hX", epos_read_indata_word(i));
  
  printk("\n");
}

module_init(debug_module_init);
module_exit(debug_module_cleanup);

