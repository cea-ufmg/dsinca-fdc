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

#include "epos.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <rtai_serial.h>

MODULE_AUTHOR("Dimas Abreu Dutra");
MODULE_DESCRIPTION("Real time device driver of Maxxon motor EPOS");
MODULE_LICENSE("GPL");

/** Module parameters **/
static int ser_port = 0;
MODULE_PARM (ser_port, "i");
MODULE_PARM_DESC (ser_port, "The rtai_serial serial port number. Integer value (default 0)");

static int baud = 38400;

module_init(epos_init);
module_exit(epos_cleanup);

static int __init epos_init() {
  int err;
  /*
  err = rt_spopen(ser_port, unsigned int baud, unsigned int numbits,
               unsigned int stopbits, unsigned int parity, int mode,
               int fifotrig)
  */
  return 0;
}

static void __exit my_cleanup() {
}

