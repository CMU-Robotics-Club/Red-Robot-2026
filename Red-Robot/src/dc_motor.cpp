#include <Arduino.h>

#include "dc_motor.h"
#include "driver/mcpwm.h"
#include "freertos/semphr.h"
#include "pinouts.h"

#define MOTOR_PWM_FREQ 20000

namespace {
static SemaphoreHandle_t dc_mutex = NULL;

static const int dc_en[] = {MOTOR0_EN, MOTOR1_EN, MOTOR2_EN, MOTOR3_EN};
static const int dc_phase[] = {MOTOR0_PHASE, MOTOR1_PHASE, MOTOR2_PHASE,
                               MOTOR3_PHASE};

// Each motor gets its own MCPWM timer, output A
struct McpwmMap {
  mcpwm_unit_t unit;
  mcpwm_timer_t timer;
  mcpwm_io_signals_t signal;
};

static const McpwmMap mc[] = {
    {MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A},
    {MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM1A},
    {MCPWM_UNIT_0, MCPWM_TIMER_2, MCPWM2A},
    {MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM0A},
};
} // namespace

namespace DCMotor {

void init() {
  if (dc_mutex == NULL) {
    dc_mutex = xSemaphoreCreateMutex();
  }

  Serial.println("[DC_MOTOR] Initializing channels...");

  for (int i = 0; i < 2; i++) {
    // 1. Route the MCPWM output signal to the EN pin
    mcpwm_gpio_init(mc[i].unit, mc[i].signal, dc_en[i]);

    // 2. Configure the timer/operator
    mcpwm_config_t cfg = {};
    cfg.frequency = MOTOR_PWM_FREQ;
    cfg.cmpr_a = 0.0f; // duty in percent
    cfg.cmpr_b = 0.0f;
    cfg.counter_mode = MCPWM_UP_COUNTER;
    cfg.duty_mode = MCPWM_DUTY_MODE_0; // active-high
    mcpwm_init(mc[i].unit, mc[i].timer, &cfg);

    // 3. Configure PHASE pin
    pinMode(dc_phase[i], OUTPUT);
    digitalWrite(dc_phase[i], LOW);

    Serial.printf("[DC_MOTOR] Motor %d -> EN Pin: %d (MCPWM unit %d, timer "
                  "%d), PHASE Pin: %d\n",
                  i, dc_en[i], (int)mc[i].unit, (int)mc[i].timer, dc_phase[i]);
  }
}

void write(DCMOTOR d, int16_t speed) {
  if (dc_mutex == NULL)
    return;

  if (xSemaphoreTake(dc_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    Serial.println("[DC_MOTOR] Error: Mutex lock timeout!");
    return;
  }

  int motor = static_cast<int>(d);
  speed = constrain(speed, -100, 100);

  if (speed == 0) {
    mcpwm_set_duty(mc[motor].unit, mc[motor].timer, MCPWM_OPR_A, 0.0f);
    digitalWrite(dc_phase[motor], LOW);
    Serial.printf("[DC_MOTOR] Motor %d STOP (Duty: 0, Phase: LOW)\n", motor);
    xSemaphoreGive(dc_mutex);
    return;
  }

  // Set PHASE (Direction): LOW = Forward, HIGH = Reverse
  uint8_t phase_state = (speed > 0) ? LOW : HIGH;
  digitalWrite(dc_phase[motor], phase_state);

  // Set ENABLE (PWM duty): MCPWM takes percent directly, so no mapping needed
  float duty = (float)abs(speed);
  mcpwm_set_duty(mc[motor].unit, mc[motor].timer, MCPWM_OPR_A, duty);

  Serial.printf(
      "[DC_MOTOR] Motor %d | Target Speed: %d | Duty: %d%% | Phase Pin: %s\n",
      motor, speed, (int)abs(speed), phase_state == LOW ? "LOW" : "HIGH");

  xSemaphoreGive(dc_mutex);
}

} // namespace DCMotor