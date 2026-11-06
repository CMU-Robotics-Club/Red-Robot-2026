#include <Arduino.h>
#include <RedRobot.h>


// initialize any variables here
void setup() {
  RedRobot::InitRedRobot();         // call this first
  RedRobot::InitDCMotors();
  RedRobot::InitServo();
  RedRobot::InitLineSensors();
  RedRobot::InitAuton();            // call this to activate the AutonomousLoop
}

void AutonomousLoop() {
    static uint32_t lastPrint = 0;
    static bool started = false;

    if (!started) { // Ex: get out of the starting box
        started = true;
        RedRobot::WriteDCMotor(2, 100);
        RedRobot::WriteDCMotor(3, 100);
        delay(1000);

        // Turn right
        RedRobot::WriteDCMotor(2, 100);
        RedRobot::WriteDCMotor(3, -100);
        delay(500);
        RedRobot::WriteDCMotor(2, 0);
        RedRobot::WriteDCMotor(3, 0);

        // line follower example
    }
    
    // Ex: Repeatedly move the servo and get in scoring position
    RedRobot::SetServo(0, 0);
    RedRobot::SetServo(1, 0);
    RedRobot::SetServo(2, 0);
    RedRobot::SetServo(3, 0);
    RedRobot::WriteDCMotor(2, 100);
    RedRobot::WriteDCMotor(3, 100);
    delay(1000);
    RedRobot::WriteDCMotor(2, 0);
    RedRobot::WriteDCMotor(3, 0);
    RedRobot::SetServo(0, 90);
    RedRobot::SetServo(1, 180);
    RedRobot::SetServo(2, 90);
    RedRobot::SetServo(3, 180);
    delay(500);
    RedRobot::WriteDCMotor(2, -100);
    RedRobot::WriteDCMotor(3, -100);
    delay(1000);


    if (millis() - lastPrint >= 500) {
        lastPrint = millis();
        Serial.println("[AUTO] Running");
    }
}
