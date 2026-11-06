#include <Arduino.h>
#include <RedRobot.h>


void setup() {
  RedRobot::InitRedRobot();
  RedRobot::InitDCMotors();
  RedRobot::InitServo();
  RedRobot::InitLineSensors();

}

void AutonomousLoop() {
  RedRobot::WriteDCMotor(2, 100);
  RedRobot::WriteDCMotor(3, 100);
//  RedRobot::StartRemoteControl();
}