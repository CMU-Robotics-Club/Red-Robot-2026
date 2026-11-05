#include <Arduino.h>
#include "RedRobot.h"


void setup() {
  RedRobot::InitRedRobot();
  RedRobot::InitDCMotors();
  RedRobot::InitServo();
  RedRobot::InitLineSensors();

}

void loop() {
  RedRobot::StartRemoteControl();
}