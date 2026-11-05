#include <Arduino.h>
#include "RedRobot.h"
#include "pinouts.h"
#include "servo.h"
#include "dc_motor.h"
#include "servo.h"
#include "display.h"
#include "ble.h"

#define VOLTAGE_BUF_SIZE 60
volatile int16_t __RR_global_motor_speed[4] = { 0 };

namespace Battery {
float read() {
  // R26 = 100k, R27 = 33k
  // so Vin = Vout * ((R26+R27)/R27) = Vout * 4.0303
  uint32_t mv = analogReadMilliVolts(ADC1_0);
  float volts = mv / 1000.0; 
  return volts * 4.0303;
}
} // namespace Battery

namespace LineSensors {
const int LINE_SENSOR_PINS[] = { LF0, LF1, LF2, LF3, LF4, LF5 };
void init() {
  pinMode(LF_LEDCTRL, OUTPUT);
  digitalWrite(LF_LEDCTRL, LOW);
}

void read(uint16_t values[6]) {
  digitalWrite(LF_LEDCTRL, HIGH);
  vTaskDelay(pdMS_TO_TICKS(1));
  for (int i = 0; i < 6; ++i) {
    pinMode(LINE_SENSOR_PINS[i], OUTPUT);
    digitalWrite(LINE_SENSOR_PINS[i], HIGH);
    vTaskDelay(pdMS_TO_TICKS(1));
    pinMode(LINE_SENSOR_PINS[i], INPUT);
    unsigned long long start = micros();
    while (digitalRead(LINE_SENSOR_PINS[i]) && micros() - start < 10000) {}
    unsigned long long end = micros();

    uint16_t diff = end - start;

    values[i] = diff;
  }
  digitalWrite(LF_LEDCTRL, LOW);
}

} // namespace LineSensors

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
    int16_t last_speed[4] = {-999, -999, -999, -999 };

    while (1) {
        // Only update and print when speed changes to reduce log spam
        for (uint8_t i = 0; i < 4; i++) {
        int16_t local_motor_speed = __RR_global_motor_speed[i];
        if (local_motor_speed != last_speed[i]) {
            DCMotor::write(DCMotor::DCMOTOR(i), local_motor_speed);
            last_speed[i] = local_motor_speed;
        }
        }
    }
}

namespace RedRobot {
void InitRedRobot() {
  pinMode(MOTOR1_EN, OUTPUT);
  digitalWrite(MOTOR1_EN, LOW);
  Serial.begin(115200);

  delay(2000);

  Serial.printf("Reset reason: %d\n", (int)esp_reset_reason());

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Display::init();
  BLE::init();

  xTaskCreatePinnedToCore(batteryLevelTask, "batteryLevel", 2048, nullptr, 1, nullptr, 1);
}
void InitDCMotors() {
    DCMotor::init();
}
void InitServo() {
    Servo::init();
}
void InitLineSensors() {
  LineSensors::init();
}

/**
 * @brief Drive a certain motor at a certain speed (-100, 100)
 */
void WriteDCMotor(uint8_t d, int16_t speed) {
    DCMotor::write(DCMotor::DCMOTOR(d), speed);
}

void WriteServo(uint8_t s, int16_t angle) {
    Servo::write(Servo::SERVO(s), angle);
}

void ReadLineSensor(uint16_t values[6]) {
    LineSensors::read(values);

}

void StartRemoteControl() {
    xTaskCreatePinnedToCore(servoTask, "servo", 2048, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(motorTask, "dc_motor", 4096, nullptr, 1, nullptr, 1);
}
}