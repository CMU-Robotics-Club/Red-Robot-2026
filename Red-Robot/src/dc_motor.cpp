#include <Arduino.h>
#include "dc_motor.h"
#include "freertos/semphr.h"
#include "pinouts.h"

#define MOTOR_PWM_FREQ 20000
#define MOTOR_PWM_RES 10
#define LEDC_MAX_DUTY 1023

static SemaphoreHandle_t dc_mutex = NULL;

static const int dc_en[] = {MOTOR0_EN, MOTOR1_EN, MOTOR2_EN, MOTOR3_EN};
static const int dc_phase[] = {MOTOR0_PHASE, MOTOR1_PHASE, MOTOR2_PHASE, MOTOR3_PHASE};

namespace DCMotor {

void init() {
  if (dc_mutex == NULL) {
    dc_mutex = xSemaphoreCreateMutex();
  }

  Serial.println("[DC_MOTOR] Initializing channels...");

  for (int i = 0; i < 2; i++) {
    int channel = i + 4; // PWM channels 4-5

    // 1. Configure the LEDC PWM channel
    ledcSetup(channel, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcAttachPin(dc_en[i], channel);

    // 2. Configure PHASE pin
    pinMode(dc_phase[i], OUTPUT);
    digitalWrite(dc_phase[i], LOW);

    Serial.printf("[DC_MOTOR] Motor %d -> EN Pin: %d (PWM Ch: %d), PHASE Pin: %d\n", 
                  i, dc_en[i], channel, dc_phase[i]);
  }
}

void write(DCMOTOR d, int16_t speed) {
  if (dc_mutex == NULL) return;

  if (xSemaphoreTake(dc_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    Serial.println("[DC_MOTOR] Error: Mutex lock timeout!");
    return;
  }

  int motor = static_cast<int>(d);
  int channel = motor + 4;

  speed = constrain(speed, -100, 100);

  if (speed == 0) {
    ledcWrite(channel, 0);
    digitalWrite(dc_phase[motor], LOW);
    Serial.printf("[DC_MOTOR] Motor %d STOP (Duty: 0, Phase: LOW)\n", motor);
    xSemaphoreGive(dc_mutex);
    return;
  }

  // Set PHASE (Direction): LOW = Forward, HIGH = Reverse
  uint8_t phase_state = (speed > 0) ? LOW : HIGH;
  digitalWrite(dc_phase[motor], phase_state);

  // Set ENABLE (PWM Duty Cycle)
  uint32_t duty = map(abs(speed), 0, 100, 0, LEDC_MAX_DUTY);
  ledcWrite(channel, duty);

  Serial.printf("[DC_MOTOR] Motor %d | Target Speed: %d | Duty: %u/%d | Phase Pin: %s\n",
                motor, speed, duty, LEDC_MAX_DUTY, phase_state == LOW ? "LOW" : "HIGH");

  xSemaphoreGive(dc_mutex);
}

} // namespace DCMotor