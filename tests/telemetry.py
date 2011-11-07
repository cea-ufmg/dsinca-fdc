#! /usr/bin/env python

'''Reads telemetry from the flight data computer's modem.'''

from ctypes import c_float, c_uint8, c_int16, c_int64, Structure
from io import open, SEEK_CUR

class ModemMsg(Structure):
    _fields_ = [('daq', c_float * 16),
                ('gps_latitude', c_float),
                ('gps_longitude', c_float),
                ('gps_altitude', c_float),
                ('gps_nvel', c_float),
                ('gps_evel', c_float),
                ('gps_dvel', c_float),
                ('nav_angle', c_float * 3),
                ('nav_gyro', c_float * 3),
                ('nav_accel', c_float * 3),
                ('nav_nvel', c_float),
                ('nav_evel', c_float),
                ('nav_dvel', c_float),
                ('nav_latitude', c_float),
                ('nav_longitude', c_float),
                ('nav_altitude', c_float),
                ('pitot_static', c_float),
                ('pitot_temperature', c_float),
                ('pitot_dynamic', c_float),
                ('pitot_aoa', c_float),
                ('pitot_sideslip', c_float),
                ('tstamp', c_int64),
                ('crc', c_uint8)]

def read_modem(port):
    h1,h0 = port.read(2)
    while h1+h0 != 'TM':
        h1 = h0
        h0 = port.read(1)
    
    #Consume the padding
    port.read(2)
    
    msg = ModemMsg()
    port.readinto(msg)
    
    return msg

if __name__ == '__main__':
    port = open('/dev/ttyUSB0', 'rb')
