/*
        modem.c - Serial modem real time device driver (module)
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
 * Serial modem device driver, responsible for telemetry and control.
 */

#include "modem.h"

#include <stdint.h>

#include <asm/div64.h>
#include <linux/crc8.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/time.h>

#include <rtai_sched.h>
#include <rtai_serial.h>

MODULE_AUTHOR("Dimas Abreu Dutra");
MODULE_DESCRIPTION("Real time device driver of Maxon motor EPOS");
MODULE_LICENSE("GPL");

/** Module parameters **/
static int ser_port = 5;
MODULE_PARM (ser_port, "i");
MODULE_PARM_DESC (ser_port, "The rtai_serial serial port number. "
          "Integer value (default 5)");

static int baud = 115200;
MODULE_PARM (baud, "i");
MODULE_PARM_DESC (baud, "The baud rate. Integer value (default 115200)");

/** Module definitions **/
//Serial communication parameters
#define STARTBITS 1
#define DATABITS 8
#define PARITY RT_SP_PARITY_NONE
#define STOPBITS 1
#define HARDCTRL RT_SP_NO_HAND_SHAKE
#define FIFOTRIG RT_SP_FIFO_SIZE_8

//MSB-first crc-8 (polynomial x^8 + x^7 + x^6 + x^4 + x^2 + 1, a.k.a. 0xD5)
DECLARE_CRC8_TABLE(crc_table);

/** Internal functions **/
static void errmsg(char * msg);

/** Module code **/
static int __init modem_init() {
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
  
  crc8_populate_msb(crc_table, 0xD5);
  
 spopen_fail: return err;
}

static void __exit modem_cleanup() {
  if (rt_spclose(ser_port) == -ENODEV)
    errmsg("Error closing serial: rtai_serial claims port does not exist.");
}

module_init(modem_init);
module_exit(modem_cleanup);

void modem_send_daq_data(const msg_daq_t *daq_msg){
  u8 crc;
  uint16_t header = 0x4441; //The characters "AD"
  int32_t timestamp;//The "uptime" in microseconds

  if (rt_spget_txfrbs(ser_port) < 2 + 4*16 + 4 + 1) {
    errmsg("serial buffer full.");
    return;
  }

  {
    int64_t time_sys = daq_msg->time_sys;
    do_div(time_sys, 1000000);
    timestamp = (int32_t) time_sys;
  }

  rt_spwrite(ser_port, (char*)&header, -sizeof(header));
  rt_spwrite(ser_port, (char*)daq_msg->tensao, -sizeof(daq_msg->tensao));
  rt_spwrite(ser_port, (char*)&timestamp, -sizeof(timestamp));

  crc = crc8(crc_table, (u8*) &header, sizeof(header), 0);
  printk("header crc: %d\n", (int)crc);
  crc = crc8(crc_table, (u8*) daq_msg->tensao, sizeof(daq_msg->tensao), crc);
  crc = crc8(crc_table, (u8*) &timestamp, sizeof(timestamp), crc);
  
  rt_spwrite(ser_port, (char*)&crc, -sizeof(crc));
}

void modem_send_gps_data(const msg_gps_t *gps_msg){
  /*
  modem_msg.gps_latitude = gps_msg->latitude;
  modem_msg.gps_longitude = gps_msg->longitude;
  modem_msg.gps_altitude = gps_msg->altitude;
  modem_msg.gps_nvel = gps_msg->north_v;
  modem_msg.gps_evel = gps_msg->east_v;
  modem_msg.gps_dvel = -gps_msg->up_v;
  */
}

void modem_send_nav_data(const msg_nav_t *nav_msg){
  /*
  memcpy(&modem_msg.nav_angle, &nav_msg->angle, sizeof(modem_msg.nav_angle));
  memcpy(&modem_msg.nav_gyro, &nav_msg->gyro, sizeof(modem_msg.nav_gyro));
  memcpy(&modem_msg.nav_accel, &nav_msg->accel, sizeof(modem_msg.nav_accel));
  
  modem_msg.nav_nvel = nav_msg->nVel;
  modem_msg.nav_evel = nav_msg->eVel;
  modem_msg.nav_dvel = nav_msg->dVel;
  
  modem_msg.nav_latitude = nav_msg->latitude;
  modem_msg.nav_longitude = nav_msg->longitude;
  modem_msg.nav_altitude = nav_msg->altitude;
  */
}

void modem_send_pitot_data(const msg_pitot_t *pitot_msg){
  /*
  modem_msg.pitot_static = pitot_msg->static_pressure;
  modem_msg.pitot_temperature = pitot_msg->temperature;
  modem_msg.pitot_dynamic = pitot_msg->dynamic_pressure;
  modem_msg.pitot_aoa = pitot_msg->attack_angle;
  modem_msg.pitot_sideslip = pitot_msg->sideslip_angle;
  */
}

void modem_transmit(){
  /*
  modem_msg.tstamp = rt_get_time_ns();
  modem_msg.crc = crc7(0, (u8*) &modem_msg, sizeof(modem_msg) - 1);
  
  if (rt_spwrite(ser_port, (char*)&modem_msg, -sizeof(modem_msg)))
    errmsg("serial buffer full.");
  */
}


static void errmsg(char* msg){
  printk("Modem driver: %s\n",msg);
}
