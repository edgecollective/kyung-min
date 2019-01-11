from serial import Serial
from enum import Enum

import time

class Direction(Enum):
    forward = 'F'
    backward = 'B'

class DigitalState(Enum):
    on = 'ON'
    off = 'OFF'

class Core:
    def __init__(self, port, baud=9600):
        self.serial = Serial(port, baudrate=baud, dsrdtr=True)
        self.serial.flushInput()

    def _send(self, message):
        self.serial.write(f'{message}\n'.encode('latin-1'))
               
    def stepper(self, steps, speed=10, direction=Direction.forward):
        self._send(f'STEP 0 {steps} {speed} {direction.value}')

    def digital(self, state):
        self._send(f'DIGITAL 0 {state.value}')
        
    def get_temp(self):
        self._send(f'TEMP?')
        serial_data = self.serial.readline().strip()
        return float(serial_data)
        