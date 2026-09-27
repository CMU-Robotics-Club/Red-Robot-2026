#include <Arduino.h>
#include "pinouts.h"
#include "display.h"
#include "battery.h"
#include "buzzer.h"
#include "servo.h"

void batteryLevelTask(void *)
{
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(5));
    uint16_t val = static_cast<uint16_t>((Battery::read() * 100.0f) + 0.5f);
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

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  analogReadResolution(12);

  Display::init();
  Buzzer::init();
  Servo::init();

  xTaskCreatePinnedToCore(batteryLevelTask, "batteryLevel", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(buzzerTask, "buzzer", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(servoTask, "servo", 2048, nullptr, 1, nullptr, 1);
}

void loop()
{
  // put your main code here, to run repeatedly:
}
