// Demo Code for SerialCommand Library
// Craig Versek, Jan 2014
// based on code from Steven Cogswell, May 2011

// #define WITH_DISPLAY

#define DEBOUNCE_INTERVAL_SERVO 200
#define DEBOUNCE_INTERVAL_STEPPER 20

#include "Adafruit_TMP007.h"
#include "Adafruit_VL53L0X.h"
#include <Adafruit_MotorShield.h>
#include <SerialCommand.h>
#include <Servo.h>
#include <Wire.h>

#ifdef WITH_DISPLAY

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

#endif

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

#define NUM_DIGITAL_CHANNELS 3
const int init_digital_channels[] = { 22, 23, 24 };

typedef struct ex_stepper_t {
  unsigned char direction_pin;
  unsigned char pulse_pin;
} ex_stepper;

const ex_stepper ex_stepper_channels[] = {
  { 15, 14 }
};

#define NUM_BUTTONS 8
const unsigned char button_pins[] = { 5, 6, 9, 10, 11, 17, 18, 19 };

static volatile unsigned char poll_17_state = 1;
static volatile unsigned char poll_18_state = 1;
static volatile unsigned char poll_19_state = 1;

static volatile unsigned long last_interrupt_time = 0;
static volatile short servo_0_angle = 0;
static volatile short servo_1_angle = 0;

static volatile unsigned char has_temp = 1;
static volatile unsigned char has_dist = 1;

#ifdef WITH_DISPLAY
static volatile unsigned char should_update_display = 1;
volatile float last_temp = 0;
volatile float last_dist = 0;
#endif

void setup() {

#ifdef WITH_DISPLAY
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.display();
#endif

  for (int i = 0; i < NUM_DIGITAL_CHANNELS; i++) {
    pinMode(init_digital_channels[i], OUTPUT);      // Configure the onboard LED for output
    digitalWrite(init_digital_channels[i], HIGH);    // default to LED off
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(button_pins[i], INPUT);
  }

  Serial.begin(9600);


  servo_0.attach(13);
  servo_0.write(0);

  servo_1.attach(12);
  servo_1.write(0);

  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz

  if (!tmp007.begin()) {
    Serial.println("No temperature sensor found");
    has_temp = 0;
  }
  digitalWrite(22, LOW);


  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    has_dist = 0;
  }
  digitalWrite(23, LOW);


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

  // Button Interrupts
  // N.B. We don't interrupt on the last 3 because
  // they end up using the same interrupts as these other pins
  attachInterrupt(digitalPinToInterrupt(5), btn_0_fall, FALLING);
  attachInterrupt(digitalPinToInterrupt(6), btn_1_fall, FALLING); // Intereferes with 17?
  attachInterrupt(digitalPinToInterrupt(9), btn_2_fall, FALLING);
  attachInterrupt(digitalPinToInterrupt(10), btn_3_fall, FALLING);
  attachInterrupt(digitalPinToInterrupt(11), btn_4_fall, FALLING);

  digitalWrite(24, LOW);
  Serial.println("Ready");
}

#ifdef WITH_DISPLAY

void update_display() {
  // text display tests
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  char buffer[50] = { 0 };


  display.println("Ready.");

  if (has_temp) {
    sprintf(buffer, "Temp: %f", last_temp);
    display.println(buffer);
    memset(buffer, 0, sizeof(buffer));
  }

  if (has_dist) {
    sprintf(buffer, "Dist: %f", last_dist);
    display.println(buffer);
    memset(buffer, 0, sizeof(buffer));
  }


  display.setCursor(0, 0);
  display.display(); // actually display all of the above
}

#endif

void loop() {
  int num_bytes = sCmd.readSerial();      // fill the buffer
  if (num_bytes > 0) {
    sCmd.processCommand();  // process the command
  }

#ifdef WITH_DISPLAY

  if (should_update_display) {
    update_display();
    should_update_display = 0;
  }

#endif

  auto state_17 = digitalRead(17);
  auto state_18 = digitalRead(18);
  auto state_19 = digitalRead(19);

  if (!state_17 && poll_17_state) {
    btn_7_fall();
  }

  if (!state_18 && poll_18_state) {
    btn_6_fall();
  }

  if (!state_19 && poll_19_state) {
    btn_5_fall();
  }

  poll_17_state = state_17;
  poll_18_state = state_18;
  poll_19_state = state_19;

  delay(10);
}

void hello_command(SerialCommand scmd) {
  scmd.println("Hello!");
}

void servo_command(SerialCommand scmd) {
  char *chan_s = scmd.next();
  char *angle_s = scmd.next();

  if (!(chan_s && angle_s)) {
    scmd.println("SERVO must have both <channel> and <angle> args");
    return;
  }

  int chan, angle;
  chan = atoi(chan_s);
  angle = atoi(angle_s);

  chan--;

  if (chan < 0 || chan > SERVO_CHANNEL_COUNT) {
    scmd.println("SERVO Channel must be 1.");
    return;
  }

  servo_chanels[chan].write(angle);
  scmd.println("OK.");
}

void digital_command(SerialCommand scmd) {
  char *chan_s = scmd.next();
  char *state_s = scmd.next();

  if (!(chan_s && state_s)) {
    scmd.println("DIGITAL must have both <channel> and <state> args");
    return;
  }

  String state = String(state_s);

  int chan = -1;
  chan = atoi(chan_s);

  if (state == String("ON")) {
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

  if (!(chan_s && steps_s && speed_s && direction_s)) {
    scmd.println("STEP must have four arguments: <channel>, <steps>, <speed>, and <direction>");
    return;
  }

  int chan, steps, speed;
  chan = atoi(chan_s);
  steps = atoi(steps_s);
  speed = atoi(speed_s);

  chan--;
  if (chan < 0 || chan > MOTOR_CHANNEL_COUNT) {
    scmd.println("Motor channel must be 1 or 2.");
    return;
  }

  Adafruit_StepperMotor *motor = motor_channels[chan];

  motor->setSpeed(speed); //RPM

  if (*direction_s == 'F') {
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

  if (!(chan_s && steps_s && speed_s && direction_s)) {
    scmd.println("EXSTEP must have four arguments: <channel>, <steps>, <speed>, and <direction>");
    return;
  }

  int chan, steps, speed;
  chan = atoi(chan_s);
  steps = atoi(steps_s);
  speed = atoi(speed_s);

  chan--;
  if (chan < 0 || chan > MOTOR_CHANNEL_COUNT) {
    scmd.println("Motor channel must be 1 or 2.");
    return;
  }

  const ex_stepper motor = ex_stepper_channels[chan];

  digitalWrite(motor.direction_pin, *direction_s == 'F');

  speed = max(1000 - speed, 1);

  for (int i = 0; i < steps; i++) {
    digitalWrite(motor.pulse_pin, HIGH);
    delayMicroseconds(speed);
    digitalWrite(motor.pulse_pin, LOW);
    delayMicroseconds(speed);
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

////// Button Interrupt Handlers ///////////////////////////////////////////////

void btn_0_fall() {
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > DEBOUNCE_INTERVAL_STEPPER) {
    motor_channels[0]->step(50, BACKWARD, SINGLE);
  }
  last_interrupt_time = interrupt_time;
}

void btn_1_fall() {
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > DEBOUNCE_INTERVAL_STEPPER) {
    motor_channels[0]->step(50, FORWARD, SINGLE);
  }
  last_interrupt_time = interrupt_time;
}

void btn_2_fall() {
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > DEBOUNCE_INTERVAL_STEPPER) {
    motor_channels[1]->step(50, FORWARD, SINGLE);
    last_interrupt_time = interrupt_time;
  }
}

void btn_3_fall() {
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > DEBOUNCE_INTERVAL_STEPPER) {
    motor_channels[1]->step(50, BACKWARD, SINGLE);
  }
  last_interrupt_time = interrupt_time;
}

void btn_4_fall() {
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > DEBOUNCE_INTERVAL_SERVO) {
    servo_0_angle += 45;
    if (servo_0_angle > 180) {
      servo_0_angle = 0;
    }

    servo_0.write(servo_0_angle);
    last_interrupt_time = interrupt_time;
  }
}

void btn_5_fall() {
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > DEBOUNCE_INTERVAL_SERVO) {
    servo_0_angle -= 45;

    if (servo_0_angle < 0) {
      servo_0_angle = 180;
    }

    servo_0.write(servo_0_angle);
    last_interrupt_time = interrupt_time;
  }
}

void btn_6_fall() {
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > DEBOUNCE_INTERVAL_SERVO) {
    servo_1_angle += 45;
    if (servo_1_angle > 180) {
      servo_1_angle = 0;
    }

    servo_1.write(servo_1_angle);
    last_interrupt_time = interrupt_time;
  }

}

void btn_7_fall() {
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > DEBOUNCE_INTERVAL_SERVO) {
    servo_1_angle -= 45;

    if (servo_1_angle < 0) {
      servo_1_angle = 180;
    }

    servo_1.write(servo_1_angle);

#ifdef WITH_DISPLAY
    last_temp = tmp007.readObjTempC();
    should_update_display = 1;
#endif

    last_interrupt_time = interrupt_time;
  }

}
