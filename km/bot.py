from serial import Serial
from enum import Enum

import time

class Direction(Enum):
    forward = 'F'
    backward = 'B'
    
class Status(Enum):
    on = 'ON'
    off = 'OFF'

class Core:
    def __init__(self, port, baud=9600):
        self.serial = Serial(port, baudrate=baud, dsrdtr=True)
        self.serial.flushInput()

# utilities

    def _send(self, message):
        self.serial.write(message.encode('latin-1'))
    
    def _read(self):
        serial_data = self.serial.readline()
        return serial_data
        
# commands

    def stepper(self,steps,speed=100,direction=Direction.forward):
        msg=str(direction.value)+" "+str(steps)+"\n"
        self._send(msg)
        print(self._read())
        
    def led(self,status=Status.on):
        msg=str(status.value)+"\n"
        self._send(msg)
        print(self._read())
        
    def pinout(self,pin_number=13,status=Status.on):
        if (status == Status.on):
            pinstring="1"
        else:
            pinstring="0"
        msg="PINOUT "+str(pin_number)+" "+pinstring+"\n"
        self._send(msg)
        print(self._read())
        
    def get_temp(self):
        msg="TEMP?\n"
        self._send(msg)
        temp=self._read().strip()
        return float(temp)
        
