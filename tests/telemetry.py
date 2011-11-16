#! /usr/bin/env python

'''Reads telemetry from the flight data computer's modem.'''

from __future__ import print_function

from ctypes import c_float, c_uint8, c_int32, Structure
from io import open, SEEK_CUR

import crcmod

crc8 = crcmod.mkCrcFun(0x1D5, rev=False, initCrc=0)

def crc_valid(msg):
    payload = msg.header + memoryview(msg).tobytes()[:-1]
    return msg.crc == crc8(payload)

class DAqMsg(Structure):
    header = b'AD'
    _pack_ = 1
    _fields_ = [('channel', c_float * 16),
                ('timestamp', c_int32),
                ('crc', c_uint8)]
    
    def dict(self):
        return dict((name, getattr(self,name)) for name,ctype in self._fields_)


headers = dict([msg.header, msg] for msg in (DAqMsg,))


def read_modem(port):
    h1,h0 = port.read(2)
    while h1+h0 not in headers.keys():
        h1 = h0
        h0 = port.read(1)
    
    msg = headers[h1+h0]()
    port.readinto(msg)
    
    return msg


if __name__ == '__main__':
    port = open('/dev/ttyUSB0', 'rb')
    while True:
        msg = read_modem(port)
        if crc_valid(msg):
            print("valid, timestamp:",msg.timestamp)
        else:
            print("INvalid!!!!! timestamp:",msg.timestamp)
    
