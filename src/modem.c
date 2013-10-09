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

// The file below defines the default serial port for our application
#include "rtai_rt_serial.h"

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
static int ser_port = MODEM_PORT;
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

void modem_send_ahrs_data(const msg_ahrs_t *ahrs_msg){
  u8 crc;
  uint16_t header = 0x4241; //The characters "AH" (little endian)
  int32_t timestamp;//The "uptime" in microseconds

  if (rt_spget_txfrbs(ser_port) < 2 + 4*3*4 + 4 + 1) {
    errmsg("serial buffer full.");
    return;
  }

  {
    int64_t time_sys = ahrs_msg->time_sys;
    do_div(time_sys, 1000000);
    timestamp = (int32_t) time_sys;
  }

  rt_spwrite(ser_port, (char*)&header, -sizeof(header));
  rt_spwrite(ser_port, (char*)ahrs_msg->angle, -sizeof(ahrs_msg->angle));
  rt_spwrite(ser_port, (char*)ahrs_msg->gyro, -sizeof(ahrs_msg->gyro));
  rt_spwrite(ser_port, (char*)ahrs_msg->accel, -sizeof(ahrs_msg->accel));
  rt_spwrite(ser_port, (char*)ahrs_msg->magnet, -sizeof(ahrs_msg->magnet));
  rt_spwrite(ser_port, (char*)&timestamp, -sizeof(timestamp));

  crc = crc8(crc_table, (u8*) &header, sizeof(header), 0);
  crc = crc8(crc_table, (u8*) ahrs_msg->angle, sizeof(ahrs_msg->angle), crc);
  crc = crc8(crc_table, (u8*) ahrs_msg->gyro, sizeof(ahrs_msg->gyro), crc);
  crc = crc8(crc_table, (u8*) ahrs_msg->accel, sizeof(ahrs_msg->accel), crc);
  crc = crc8(crc_table, (u8*) ahrs_msg->magnet, sizeof(ahrs_msg->magnet), crc);
  crc = crc8(crc_table, (u8*) &timestamp, sizeof(timestamp), crc);
  
  rt_spwrite(ser_port, (char*)&crc, -sizeof(crc));
}

void modem_send_daq_data(const msg_daq_t *daq_msg){
  u8 crc;
  uint16_t header = 0x4441; //The characters "AD" (little endian)
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
  crc = crc8(crc_table, (u8*) daq_msg->tensao, sizeof(daq_msg->tensao), crc);
  crc = crc8(crc_table, (u8*) &timestamp, sizeof(timestamp), crc);
  
  rt_spwrite(ser_port, (char*)&crc, -sizeof(crc));
}

void modem_send_gps_data(const msg_gps_t *gps_msg){
  u8 crc;
  uint16_t header = 0x5047; //The characters "GP" (little endian)
  int32_t timestamp;//The "uptime" in microseconds

  if (rt_spget_txfrbs(ser_port) < 2 + 4*6 + 4 + 1) {
    errmsg("serial buffer full.");
    return;
  }

  {
    int64_t time_sys = gps_msg->time_sys;
    do_div(time_sys, 1000000);
    timestamp = (int32_t) time_sys;
  }

  rt_spwrite(ser_port, (char*)&header, -sizeof(header));
  rt_spwrite(ser_port, (char*)&gps_msg->latitude, -sizeof(gps_msg->latitude));
  rt_spwrite(ser_port, (char*)&gps_msg->longitude, -sizeof(gps_msg->longitude));
  rt_spwrite(ser_port, (char*)&gps_msg->altitude, -sizeof(gps_msg->altitude));
  rt_spwrite(ser_port, (char*)&gps_msg->north_v, -sizeof(gps_msg->north_v));
  rt_spwrite(ser_port, (char*)&gps_msg->east_v, -sizeof(gps_msg->east_v));
  rt_spwrite(ser_port, (char*)&gps_msg->up_v, -sizeof(gps_msg->up_v));
  rt_spwrite(ser_port, (char*)&timestamp, -sizeof(timestamp));

  crc = crc8(crc_table,(u8*)&header, sizeof(header), 0);
  crc = crc8(crc_table,(u8*)&gps_msg->latitude,sizeof(gps_msg->latitude),crc);
  crc = crc8(crc_table,(u8*)&gps_msg->longitude,sizeof(gps_msg->longitude),crc);
  crc = crc8(crc_table,(u8*)&gps_msg->altitude, sizeof(gps_msg->altitude), crc);
  crc = crc8(crc_table,(u8*)&gps_msg->north_v, sizeof(gps_msg->north_v), crc);
  crc = crc8(crc_table,(u8*)&gps_msg->east_v, sizeof(gps_msg->east_v), crc);
  crc = crc8(crc_table,(u8*)&gps_msg->up_v, sizeof(gps_msg->up_v), crc);
  crc = crc8(crc_table,(u8*)&timestamp, sizeof(timestamp), crc);
  
  rt_spwrite(ser_port, (char*)&crc, -sizeof(crc));
}

void modem_send_nav_data(const msg_nav_t *nav_msg){
  u8 crc;
  uint16_t header = 0x564e; //The characters "NV" (little endian)
  int32_t timestamp;//The "uptime" in microseconds

  if (rt_spget_txfrbs(ser_port) < 2 + 4*3*5 + 4 + 1) {
    errmsg("serial buffer full.");
    return;
  }

  {
    int64_t time_sys = nav_msg->time_sys;
    do_div(time_sys, 1000000);
    timestamp = (int32_t) time_sys;
  }

  rt_spwrite(ser_port, (char*)&header, -sizeof(header));
  rt_spwrite(ser_port, (char*)nav_msg->angle, -sizeof(nav_msg->angle));
  rt_spwrite(ser_port, (char*)nav_msg->gyro, -sizeof(nav_msg->gyro));
  rt_spwrite(ser_port, (char*)nav_msg->accel, -sizeof(nav_msg->accel));
  rt_spwrite(ser_port, (char*)&nav_msg->nVel, -sizeof(nav_msg->nVel));
  rt_spwrite(ser_port, (char*)&nav_msg->eVel, -sizeof(nav_msg->eVel));
  rt_spwrite(ser_port, (char*)&nav_msg->dVel, -sizeof(nav_msg->dVel));
  rt_spwrite(ser_port, (char*)&nav_msg->latitude, -sizeof(nav_msg->latitude));
  rt_spwrite(ser_port, (char*)&nav_msg->longitude, -sizeof(nav_msg->longitude));
  rt_spwrite(ser_port, (char*)&nav_msg->altitude, -sizeof(nav_msg->altitude));
  rt_spwrite(ser_port, (char*)&timestamp, -sizeof(timestamp));

  crc = crc8(crc_table, (u8*) &header, sizeof(header), 0);
  crc = crc8(crc_table, (u8*) nav_msg->angle, sizeof(nav_msg->angle), crc);
  crc = crc8(crc_table, (u8*) nav_msg->gyro, sizeof(nav_msg->gyro), crc);
  crc = crc8(crc_table, (u8*) nav_msg->accel, sizeof(nav_msg->accel), crc);
  crc = crc8(crc_table, (u8*) &nav_msg->nVel, sizeof(nav_msg->nVel), crc);
  crc = crc8(crc_table, (u8*) &nav_msg->eVel, sizeof(nav_msg->eVel), crc);
  crc = crc8(crc_table, (u8*) &nav_msg->dVel, sizeof(nav_msg->dVel), crc);
  crc = crc8(crc_table,(u8*)&nav_msg->latitude,sizeof(nav_msg->latitude),crc);
  crc = crc8(crc_table,(u8*)&nav_msg->longitude,sizeof(nav_msg->longitude),crc);
  crc = crc8(crc_table,(u8*)&nav_msg->altitude,sizeof(nav_msg->altitude),crc);
  crc = crc8(crc_table, (u8*) &timestamp, sizeof(timestamp), crc);
  
  rt_spwrite(ser_port, (char*)&crc, -sizeof(crc));
}

void modem_send_pitot_data(const msg_pitot_t *pitot_msg){
  u8 crc;
  uint16_t header = 0x5450; //The characters "PT" (little endian)
  int32_t timestamp;//The "uptime" in microseconds

  if (rt_spget_txfrbs(ser_port) < 2 + 4*5 + 4 + 1) {
    errmsg("serial buffer full.");
    return;
  }

  {
    int64_t time_sys = pitot_msg->time_sys;
    do_div(time_sys, 1000000);
    timestamp = (int32_t) time_sys;
  }

  rt_spwrite(ser_port, (char*)&header, -sizeof(header));
  rt_spwrite(ser_port, (char*)&pitot_msg->static_pressure,
	     -sizeof(pitot_msg->static_pressure));
  rt_spwrite(ser_port, (char*)&pitot_msg->temperature,
	     -sizeof(pitot_msg->temperature));
  rt_spwrite(ser_port, (char*)&pitot_msg->dynamic_pressure,
	     -sizeof(pitot_msg->dynamic_pressure));
  rt_spwrite(ser_port, (char*)&pitot_msg->attack_angle,
	     -sizeof(pitot_msg->attack_angle));
  rt_spwrite(ser_port, (char*)&pitot_msg->sideslip_angle,
	     -sizeof(pitot_msg->sideslip_angle));
  rt_spwrite(ser_port, (char*)&timestamp, -sizeof(timestamp));

  crc = crc8(crc_table, (u8*) &header, sizeof(header), 0);
  crc = crc8(crc_table, (u8*) &pitot_msg->static_pressure,
	     sizeof(pitot_msg->static_pressure), crc);
  crc = crc8(crc_table, (u8*) &pitot_msg->temperature,
	     sizeof(pitot_msg->temperature), crc);
  crc = crc8(crc_table, (u8*) &pitot_msg->dynamic_pressure,
	     sizeof(pitot_msg->dynamic_pressure), crc);
  crc = crc8(crc_table, (u8*) &pitot_msg->attack_angle,
	     sizeof(pitot_msg->attack_angle), crc);
  crc = crc8(crc_table, (u8*) &pitot_msg->sideslip_angle,
	     sizeof(pitot_msg->sideslip_angle), crc);
  crc = crc8(crc_table, (u8*) &timestamp, sizeof(timestamp), crc);
  
  rt_spwrite(ser_port, (char*)&crc, -sizeof(crc));
}

static void errmsg(char* msg){
  printk("Modem driver: %s\n",msg);
}
