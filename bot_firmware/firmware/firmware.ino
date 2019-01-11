// Demo Code for SerialCommand Library
// Craig Versek, Jan 2014
// based on code from Steven Cogswell, May 2011


#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "Adafruit_TMP007.h"
#include <SerialCommand.h>

#define arduinoLED 13   // Arduino LED on board

SerialCommand sCmd(Serial);         // The demo SerialCommand object, initialize with any Stream object

Adafruit_TMP007 tmp007;

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *motor_0 = AFMS.getStepper(200, 2);

const int digital_channels[] = { arduinoLED };
Adafruit_StepperMotor* motor_channels[] = { motor_0 };

void setup() {
  Serial.begin(9600);

  for(int i = 0; i < 1; i++) {
    pinMode(digital_channels[i], OUTPUT);      // Configure the onboard LED for output
    digitalWrite(digital_channels[i], LOW);    // default to LED off
  }

  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz

  if (! tmp007.begin()) {
    Serial.println("No sensor found");
    while (1);
  }

  // Setup callbacks for SerialCommand commands

  // DIGITAL <channel: uint> <state: ON | OFF>
  sCmd.addCommand("DIGITAL", digital_command);

  // STEP <channel: uint> <steps: uint> <speed: uint> <direction: F | B>
  sCmd.addCommand("STEP", stepper_command);

  // TEMP?
  sCmd.addCommand("TEMP?", temp_query);

  sCmd.setDefaultHandler(unrecognized);

  Serial.println("Ready");
}

void loop() {
  int num_bytes = sCmd.readSerial();      // fill the buffer
  if (num_bytes > 0) {
    sCmd.processCommand();  // process the command
  }
  delay(10);
}

void digital_command(SerialCommand scmd) {
  char *chan_s = scmd.next();
  char *state_s = scmd.next();

  if(!(chan_s && state_s)) {
    scmd.print("DIGITAL must have both <channel> and <state> args");
    return;
  }

  String state = String(state_s);

  int chan = -1;
  chan = atoi(chan_s);

  if(state == String("ON")) {
    digitalWrite(digital_channels[chan], HIGH);
  } else {
    digitalWrite(digital_channels[chan], LOW);
  }
}


void temp_query(SerialCommand scmd) {
  float objt = tmp007.readObjTempC();
  scmd.println(objt);
}

void stepper_command(SerialCommand scmd) {
  char *chan_s = scmd.next();
  char *steps_s = scmd.next();
  char *speed_s = scmd.next();
  char *direction_s = scmd.next();

  if(!(chan_s && steps_s && speed_s && direction_s)) {
    scmd.println("STEP must have four arguments: <channel>, <steps>, <speed>, and <direction>");
    return;
  } 

  int chan, steps, speed;
  chan = atoi(chan_s);
  steps = atoi(steps_s);
  speed = atoi(speed_s);

  Adafruit_StepperMotor *motor = motor_channels[chan];

  motor->setSpeed(speed); //RPM

  if(*direction_s == 'F') {
    motor->step(steps, FORWARD, SINGLE);
  } else {
    motor->step(steps, BACKWARD, SINGLE);
  }

}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(SerialCommand this_sCmd) {
  SerialCommand::CommandInfo command = this_sCmd.getCurrentCommand();
  this_sCmd.print("Did not recognize \"");
  this_sCmd.print(command.name);
  this_sCmd.println("\" as a command.");
}
