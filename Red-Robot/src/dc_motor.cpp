#include "dc_motor.h"
#include <Arduino.h>
#include "pinouts.h"
#include "freertos/semphr.h"

#define MOTOR_PWM_FREQ 20000
#define MOTOR_PWM_RES  14
#define LEDC_MAX_DUTY 16383

static SemaphoreHandle_t dc_mutex = NULL;

static const int dc_phase[] = {MOTOR0_PHASE, MOTOR1_PHASE, MOTOR2_PHASE, MOTOR3_PHASE};
static const int dc_en[] = {MOTOR0_EN, MOTOR1_EN, MOTOR2_EN, MOTOR3_EN};

namespace DCMotor {

void init() {
  /*----------- DEBUG OUTPUT -------------*/
  pinMode(MOTOR0_EN, OUTPUT);
  pinMode(MOTOR1_EN, OUTPUT);
  pinMode(MOTOR0_PHASE, OUTPUT);
  pinMode(MOTOR1_PHASE, OUTPUT);
  return;
  /*--------------------------------------*/

  if (dc_mutex == NULL) {
    dc_mutex = xSemaphoreCreateMutex();
  }

  for (int i = 0; i < 2; i++) {
    int channel = i + 4;  // channels 4-5

    // 1. Configure the LEDC channel, frequency, and resolution
    ledcSetup(channel, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    
    // 2. Attach the physical GPIO pin to that channel
    ledcAttachPin(dc_en[i], channel);
    pinMode(dc_phase[i], OUTPUT);
  }
}

void write(DCMOTOR d, int16_t speed) {
  /*----------- DEBUG OUTPUT -------------*/
  digitalWrite(MOTOR0_PHASE, speed > 0 ? LOW : HIGH);
  digitalWrite(MOTOR1_PHASE, speed > 0 ? LOW : HIGH);

  digitalWrite(MOTOR0_EN, HIGH);
  digitalWrite(MOTOR1_EN, HIGH);
  return;
  /*--------------------------------------*/

  if (dc_mutex == NULL) return;

  if (xSemaphoreTake(dc_mutex, portMAX_DELAY) != pdTRUE)
      return;

  int motor = static_cast<int>(d);
  int channel = motor + 4;

  speed = constrain(speed, -100, 100);

  if (speed == 0) {
      ledcWrite(channel, 0);
      xSemaphoreGive(dc_mutex);
      return;
  }

  // Direction
  digitalWrite(dc_phase[motor], speed > 0 ? LOW : HIGH);

  // Magnitude
  uint32_t duty = map(abs(speed), 0, 100, 0, LEDC_MAX_DUTY);
  ledcWrite(channel, duty);
  Serial.printf("duty: %d\n", duty);
  Serial.printf("board_freq: %dMHz\n", getXtalFrequencyMhz());

  xSemaphoreGive(dc_mutex);
}

} // namespace DCMotor