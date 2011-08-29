/*
        epos.c - Maxon motor EPOS module header
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

#ifndef EPOS_H
#define EPOS_H

#include <linux/types.h>

typedef enum {
  EPOS_RESPONSE_SUCCESS,
  EPOS_RESPONSE_NONE,
  EPOS_RESPONSE_WAITING,
  EPOS_RESPONSE_ERROR
} epos_response_status_t;
extern epos_response_status_t epos_response_status;
extern int epos_num_response_words;

/**
 * @brief Write to the EPOS object dictionary.
 *
 * It is a non-blocking function call.
 *
 * @returns 0 if success, -EBUSY if the driver is already in an operation,
 * -ENOBUFS if the serial port write buffer has no space, -ENODEV if the serial
 * port number is not recognized by the rtai_serial module.
 */
int epos_write_object(u16 index, u8 subindex, u8 nodeid, u32 data);

/**
 * @brief Send a request to read from the EPOS object dictionary.
 *
 * It is a non-blocking function call.
 *
 * @returns 0 if success, -EBUSY if the driver is already in an operation,
 * -ENOBUFS if the serial port write buffer has no space, -ENODEV if the serial
 * port number is not recognized by the rtai_serial module.
 */
int epos_read_object(u16 index, u8 subindex, u8 nodeid);

/**
 * @brief Reads high and low bytes of the epos inbound message data word.
 *
 * The bytes are read from the appropriate fields of the inbound_message_payload
 * variable.
 *
 * @param windex    Index of the data word to read.
 * @param low_byte  Pointer to the variable that will hold the low byte.
 * @param high_byte Pointer to the variable that will hold the high byte.
 */
void epos_read_indata_bytes(int windex, u8 *high_byte, u8 *low_byte);

/**
 * @brief Reads epos inbound message data word.
 *
 * The word is read from the appropriate field of the inbound_message_payload 
 * variable.
 *
 * @param windex Index of the data word to read.
 *
 * @returns The data word.
 */
u16 epos_read_indata_word(int windex);

/**
 * @brief Reads epos inbound message data double word.
 *
 * The double word is read from the appropriate field of the 
 * inbound_message_payload variable.
 *
 * @param windex Index of the data word to read.
 *
 * @returns The data double word.
 */
u32 epos_read_indata_dword(int windex);

enum {
  EPOS_CONTROL_WORD_INDEX=0x6040,
  EPOS_MODES_OPERATION_INDEX=0x6060,
  EPOS_VELOCITY_MODE_SP_INDEX=0x206B,
  EPOS_POSITION_MODE_SP_INDEX=0x2062,
  EPOS_TARGET_VELOCITY_INDEX=0x60FF,
  EPOS_TARGET_POSITION_INDEX=0x607A
} epos_obj_index_t;

enum {
  EPOS_FAULT_RESET_CMD=0x0080,
  EPOS_SHUTDOWN_CMD=0x0006,
  EPOS_SWITCH_ON_CMD=0x0007,
  EPOS_ENABLE_OPERATION_CMD=0x000F,
  EPOS_HALT_CMD=0x0102,
  EPOS_GOTO_POSITION_REL_CMD=0x007F,
  EPOS_GOTO_POSITION_ABS_CMD=0x003F,
  EPOS_GOTO_VELOCITY_CMD=0x000F
} epos_commands_t;

/* Inline function helpers for some common operations */

inline int epos_fault_reset(u8 nodeid){
  return epos_write_object(EPOS_CONTROL_WORD_INDEX, 0,
			   nodeid, EPOS_FAULT_RESET_CMD);
}

inline int epos_shutdown(u8 nodeid){
  return epos_write_object(EPOS_CONTROL_WORD_INDEX, 0,
			   nodeid, EPOS_SHUTDOWN_CMD);
}

inline int epos_switch_on(u8 nodeid){
  return epos_write_object(EPOS_CONTROL_WORD_INDEX, 0,
			   nodeid, EPOS_SWITCH_ON_CMD);
}

inline int epos_enable_operation(u8 nodeid){
  return epos_write_object(EPOS_CONTROL_WORD_INDEX, 0,
			   nodeid, EPOS_ENABLE_OPERATION_CMD);
}

inline int epos_halt(u8 nodeid){
  return epos_write_object(EPOS_CONTROL_WORD_INDEX, 0,
			   nodeid, EPOS_HALT_CMD);
}

inline int epos_goto_position_rel(u8 nodeid){
  return epos_write_object(EPOS_CONTROL_WORD_INDEX, 0,
			   nodeid, EPOS_GOTO_POSITION_REL_CMD);
}

inline int epos_goto_position_abs(u8 nodeid){
  return epos_write_object(EPOS_CONTROL_WORD_INDEX, 0,
			   nodeid, EPOS_GOTO_POSITION_ABS_CMD);
}

inline int epos_goto_velocity(u8 nodeid){
  return epos_write_object(EPOS_CONTROL_WORD_INDEX, 0,
			   nodeid, EPOS_GOTO_VELOCITY_CMD);
}

typedef enum {
  EPOS_HOMING_MODE=0x06,
  EPOS_PROFILE_VELOCITY_MODE=0x03,
  EPOS_PROFILE_POSITION_MODE=0x01,
  EPOS_POSITION_MODE=0xFF,
  EPOS_VELOCITY_MODE=0xFE,
  EPOS_CURRENT_MODE=0xFD,
  EPOS_DIAGNOSTIC_MODE=0xFC,
  EPOS_MASTER_ENCODER_MODE=0xFB,
  EPOS_STEP_DIRECTION_MODE=0xFA
} epos_mode_t;

inline int epos_set_mode(u8 nodeid, epos_mode_t mode) {
  return epos_write_object(EPOS_MODES_OPERATION_INDEX, 0, nodeid, mode);
}

inline int epos_set_velocity(u8 nodeid, s32 val) {
  return epos_write_object(EPOS_VELOCITY_MODE_SP_INDEX, 0, nodeid, val);
}

inline int epos_set_position(u8 nodeid, s32 val) {
  return epos_write_object(EPOS_POSITION_MODE_SP_INDEX, 0, nodeid, val);
}

inline int epos_set_target_velocity(u8 nodeid, s32 val) {
  return epos_write_object(EPOS_TARGET_VELOCITY_INDEX, 0, nodeid, val);
}

inline int epos_set_target_position(u8 nodeid, s32 val) {
  return epos_write_object(EPOS_TARGET_POSITION_INDEX, 0, nodeid, val);
}

#endif//EPOS_H
