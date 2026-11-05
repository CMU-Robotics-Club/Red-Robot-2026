#include <Arduino.h>
#include <RedRobot.h>


void setup() {
  RedRobot::InitRedRobot();
  RedRobot::InitDCMotors();
  RedRobot::InitServo();
  RedRobot::InitLineSensors();

}

void loop() {
  /*
  while (1) {
    uint16_t sensor_values[6];

    Serial.print("Line sensors=");
    RedRobot::ReadLineSensor(sensor_values);
    for (int i = 0; i < 6; ++i)
    {
      Serial.print(sensor_values[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
  */

  RedRobot::StartRemoteControl();
}