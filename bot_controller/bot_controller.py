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
        self.serial.write(('%s\n' % message).encode('latin-1'))
               
    def stepper(self, channel, steps, speed=10, direction=Direction.forward):
        self._send('STEP %d %d %d %s' % (channel, steps, speed, direction.value))
        
        time.sleep(.2)
        serial_data = self.serial.readline().strip()
        return serial_data == b'OK.'
        
    def servo(self, channel, angle):
        self._send('SERVO %d %d' % (channel, angle))
        
        time.sleep(.2)
        serial_data = self.serial.readline().strip()
        return serial_data == b'OK.'

    def digital(self, channel, state):
        self._send('DIGITAL %d %s' % (channel, state.value))
        
        time.sleep(.2)
        serial_data = self.serial.readline().strip()
        return serial_data == b'OK.'
        
    def get_temp(self):
        self._send('TEMP?')
        
        time.sleep(.2)
        serial_data = self.serial.readline().strip()
        return float(serial_data)
        