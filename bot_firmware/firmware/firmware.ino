// Demo Code for SerialCommand Library
// Craig Versek, Jan 2014
// based on code from Steven Cogswell, May 2011


#include "Adafruit_TMP007.h"
#include "Adafruit_VL53L0X.h"
#include <Adafruit_MotorShield.h>
#include <SerialCommand.h>
#include <Servo.h>
#include <Wire.h>

SerialCommand sCmd(Serial);         // The demo SerialCommand object, initialize with any Stream object

Adafruit_TMP007 tmp007;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *motor_0 = AFMS.getStepper(200, 1);
Adafruit_StepperMotor *motor_1 = AFMS.getStepper(200, 2);

#define MOTOR_CHANNEL_COUNT 2
Adafruit_StepperMotor* motor_channels[] = { motor_0, motor_1 };

Servo servo_0;
Servo servo_1;

#define SERVO_CHANNEL_COUNT 2
Servo servo_chanels[] = { servo_0, servo_1 };

const int init_digital_channels[] = { 13 };

typedef struct ex_stepper_t {
  unsigned char direction_pin;
  unsigned char pulse_pin;
} ex_stepper;

const ex_stepper ex_stepper_channels[] = {
  { 6, 12 }
};

void setup() {
  Serial.begin(9600);

  for(int i = 0; i < 1; i++) {
    pinMode(init_digital_channels[i], OUTPUT);      // Configure the onboard LED for output
    digitalWrite(init_digital_channels[i], LOW);    // default to LED off
  }

  servo_0.attach(10);
  servo_0.write(0);

  servo_1.attach(11);
  servo_1.write(0);

  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz

  if (!tmp007.begin()) {
    Serial.println("No sensor found");
    while (1);
  }

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

  // Setup callbacks for SerialCommand commands

  sCmd.addCommand("HI", hello_command);

  // DIGITAL <PIN: uint> <state: ON | OFF>
  sCmd.addCommand("DIGITAL", digital_command);

  // STEP <channel: 1 | 2> <steps: uint> <speed: uint> <direction: F | B>
  sCmd.addCommand("STEP", stepper_command);

  // STEP <pin> <steps: uint> <speed: uint> <direction: F | B>
  sCmd.addCommand("EXSTEP", ex_stepper_command);

  // SERVO <channel> <angle: uint>
  sCmd.addCommand("SERVO", servo_command);

  // TEMP?
  sCmd.addCommand("TEMP?", temp_query);

  // DIST?
  sCmd.addCommand("DIST?", range_query);

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

void hello_command(SerialCommand scmd) {
  scmd.println("Hello!");
}

void servo_command(SerialCommand scmd) {
  char *chan_s = scmd.next();
  char *angle_s = scmd.next();

  if(!(chan_s && angle_s)) {
    scmd.println("SERVO must have both <channel> and <angle> args");
    return;
  }

  int chan, angle;
  chan = atoi(chan_s);
  angle = atoi(angle_s);

  chan--;

  if(chan < 0 || chan > SERVO_CHANNEL_COUNT) {
    scmd.println("SERVO Channel must be 1.");
    return;
  }

  servo_chanels[chan].write(angle);
  scmd.println("OK.");
}

void digital_command(SerialCommand scmd) {
  char *chan_s = scmd.next();
  char *state_s = scmd.next();

  if(!(chan_s && state_s)) {
    scmd.println("DIGITAL must have both <channel> and <state> args");
    return;
  }

  String state = String(state_s);

  int chan = -1;
  chan = atoi(chan_s);

  if(state == String("ON")) {
    digitalWrite(chan, HIGH);
  } else {
    digitalWrite(chan, LOW);
  }

  scmd.println("OK.");
}


void temp_query(SerialCommand scmd) {
  float objt = tmp007.readObjTempC();
  scmd.println(objt);
}

void range_query(SerialCommand scmd) {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    scmd.println(measure.RangeMilliMeter);
  } else {
    scmd.println("NaN");
  } 
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

  chan--;
  if(chan < 0 || chan > MOTOR_CHANNEL_COUNT) {
    scmd.println("Motor channel must be 1 or 2.");
    return;
  }

  Adafruit_StepperMotor *motor = motor_channels[chan];

  motor->setSpeed(speed); //RPM

  if(*direction_s == 'F') {
    motor->step(steps, FORWARD, SINGLE);
  } else {
    motor->step(steps, BACKWARD, SINGLE);
  }

  scmd.println("OK.");
}

void ex_stepper_command(SerialCommand scmd) {
  char *chan_s = scmd.next();
  char *steps_s = scmd.next();
  char *speed_s = scmd.next();
  char *direction_s = scmd.next();

  if(!(chan_s && steps_s && speed_s && direction_s)) {
    scmd.println("EXSTEP must have four arguments: <channel>, <steps>, <speed>, and <direction>");
    return;
  } 

  int chan, steps, speed;
  chan = atoi(chan_s);
  steps = atoi(steps_s);
  speed = atoi(speed_s);

  chan--;
  if(chan < 0 || chan > MOTOR_CHANNEL_COUNT) {
    scmd.println("Motor channel must be 1 or 2.");
    return;
  }

  const ex_stepper motor = ex_stepper_channels[chan];

  digitalWrite(motor.direction_pin, *direction_s == 'F');

  speed = max(100 - speed, 1);

  for(int i = 0; i < steps; i++) {
    digitalWrite(motor.pulse_pin, HIGH);
    delay(speed);
    digitalWrite(motor.pulse_pin, LOW);
    delay(speed);
  }

  scmd.println("OK.");
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(SerialCommand this_sCmd) {
  SerialCommand::CommandInfo command = this_sCmd.getCurrentCommand();
  this_sCmd.print("Did not recognize \"");
  this_sCmd.print(command.name);
  this_sCmd.println("\" as a command.");
}
