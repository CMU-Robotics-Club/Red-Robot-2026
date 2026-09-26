#include <Arduino.h>
#include "pinouts.h"
#include "display.h"
#include "battery.h"
#include "buzzer.h"

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

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  analogReadResolution(12);

  Display::init();
  Buzzer::init();

  xTaskCreatePinnedToCore(batteryLevelTask, "batteryLevel", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(buzzerTask, "buzzer", 2048, nullptr, 1, nullptr, 1);
}

void loop()
{
  // put your main code here, to run repeatedly:
}
