from serial import Serial
from enum import Enum

import time

class Direction(Enum):
    forward = 'F'
    backward = 'B'

class Core:
    def __init__(self, port, baud=9600):
        self.serial = Serial(port, baudrate=baud, dsrdtr=True)
        self.serial.flushInput()

    def _send(self, message):
        self.serial.write(f'{message}\n'.encode('latin-1'))
               
    def stepper(self, steps, speed=100, direction=Direction.forward):
        self._send(f'{direction.value} {steps}')
        
    def get_temp(self):
        self._send(f'TEMP?')
        serial_data = self.serial.readline().strip()
        return float(serial_data)
        