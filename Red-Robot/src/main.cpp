#include <Arduino.h>
#include "pinouts.h"
#include "display.h"
#include "battery.h"
#include "buzzer.h"
#include "servo.h"
#include "dc_motor.h"

#define VOLTAGE_BUF_SIZE 60

volatile int16_t global_motor_speed = 0;

void batteryLevelTask(void *)
{
  int voltage_buf[VOLTAGE_BUF_SIZE] = {};
  uint16_t index = 0;
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(5));
    voltage_buf[index] = static_cast<uint16_t>((Battery::read() * 100.0f) + 0.5f);
    index = (index + 1) % VOLTAGE_BUF_SIZE;

    uint16_t val = 0;
    for (int i = 0; i < VOLTAGE_BUF_SIZE; i++)
      val += voltage_buf[i];
    val /= VOLTAGE_BUF_SIZE;

    Display::show(val);
  }
}

void buzzerTask(void *) {
  Buzzer::play_startup();
  vTaskDelete(nullptr);
}

void servoTask(void *) {
  int counter = 0;
  int16_t val;
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    val = (counter % 2 == 0) ? -180 : 180;
    Servo::write(Servo::SERVO::SERVO_0, val);
    Servo::write(Servo::SERVO::SERVO_1, val);
    Servo::write(Servo::SERVO::SERVO_2, val);
    Servo::write(Servo::SERVO::SERVO_3, val);
    counter++;
  }
}

void motorTask(void *pvParameters) {
  int16_t last_speed = -999;

  while (1) {
    int16_t local_speed = global_motor_speed;

    // Only update and print when speed changes to reduce log spam
    if (local_speed != last_speed) {
      DCMotor::write(DCMotor::DCMOTOR::MOTOR_0, local_speed);
      last_speed = local_speed;
    }

    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
}

void serialTask(void *pvParameters) {
  while (1) {
    if (Serial.available() > 0) {
      int16_t incoming_speed = Serial.parseInt();

      // Clear remaining characters (\r, \n, spaces) from input buffer
      while (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r' || Serial.peek() == ' ')) {
        Serial.read();
      }

      incoming_speed = constrain(incoming_speed, -100, 100);
      Serial.printf("[SERIAL] Speed updated to: %d\n", incoming_speed);
      
      global_motor_speed = incoming_speed;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  analogReadResolution(12);

  Display::init();
  Buzzer::init();
  Servo::init();
  DCMotor::init();

  xTaskCreatePinnedToCore(batteryLevelTask, "batteryLevel", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(buzzerTask, "buzzer", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(servoTask, "servo", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(motorTask, "dc_motor", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(serialTask, "serial", 2048, nullptr, 1, nullptr, 1);
}

void loop()
{
}
