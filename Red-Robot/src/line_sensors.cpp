#include <Arduino.h>

#include "line_sensors.h"
#include "pinouts.h"
#include <stdint.h>

const int LINE_SENSOR_PINS[] = { LF0, LF1, LF2, LF3, LF4, LF5 };

namespace LineSensors {
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
