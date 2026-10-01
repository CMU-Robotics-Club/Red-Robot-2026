#include <Arduino.h>

#include "battery.h"
#include "buzzer.h"
#include "dc_motor.h"
#include "display.h"
#include "pinouts.h"
#include "servo.h"
#include "ble.h"
#include "line_sensors.h"

#define VOLTAGE_BUF_SIZE 60

volatile int16_t global_motor_speed = 0;

void batteryLevelTask(void *) {
  int voltage_buf[VOLTAGE_BUF_SIZE] = {};
  uint16_t index = 0;
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(5));
    voltage_buf[index] =
        static_cast<uint16_t>((Battery::read() * 100.0f) + 0.5f);
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
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    val = (counter % 2 == 0) ? -180 : 180;
    Servo::write(Servo::SERVO_0, val);
    Servo::write(Servo::SERVO_1, val);
    Servo::write(Servo::SERVO_2, val);
    Servo::write(Servo::SERVO_3, val);
    counter++;
  }
}

void motorTask(void *pvParameters) {
  int16_t last_speed = -999;

  while (1) {
    int16_t local_speed = global_motor_speed;

    // Only update and print when speed changes to reduce log spam
    if (local_speed != last_speed) {
      DCMotor::write(DCMotor::MOTOR_0, local_speed);
      last_speed = local_speed;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Display::init();
  Buzzer::init();
  Servo::init();
  DCMotor::init();
  BLE::init();
  LineSensors::init();

  xTaskCreatePinnedToCore(batteryLevelTask, "batteryLevel", 2048, nullptr, 1,
                          nullptr, 1);
  xTaskCreatePinnedToCore(buzzerTask, "buzzer", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(servoTask, "servo", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(motorTask, "dc_motor", 2048, nullptr, 1, nullptr, 1);
}

void loop() {
  uint16_t sensor_values[6];

  Serial.print("Line sensors=");
  LineSensors::read(sensor_values);
  for (int i = 0; i < 6; ++i)
  {
    Serial.print(sensor_values[i]);
    Serial.print(" ");
  }
  Serial.println();
}