#include "buzzer.h"
#include "pinouts.h"
#include <Arduino.h>

namespace {
const uint8_t buzzer_channel = 0;
const uint32_t buzzer_freq = 5000; // dummy value
const uint8_t buzzer_resolution = 8;

const int startupNotes[] = {523, 659, 784, 1047, 1319, 1568, 2093, 2637, 3136, 4186};
const int noteDuration = 35;
} // namespace

namespace Buzzer {
void init() {
  ledcSetup(buzzer_channel, buzzer_freq, buzzer_resolution);
  ledcAttachPin(BUZZER, buzzer_channel);
}

void play_startup() {
  int totalNotes = sizeof(startupNotes) / sizeof(int);

  for (int i = 0; i < totalNotes; i++) {
    // Generate tone using the CHANNEL number, not the GPIO pin
    ledcWriteTone(buzzer_channel, startupNotes[i]);
    ledcWrite(buzzer_channel, 50); // reduce duty cycle to prevent noise issues

    vTaskDelay(pdMS_TO_TICKS(noteDuration));

    // Stop sound between notes
    ledcWriteTone(buzzer_channel, 0);
    vTaskDelay(pdMS_TO_TICKS(30));
  }
}
} // namespace Buzzer