#include "servo.h"
#include <Arduino.h>
#include "pinouts.h"
#include "freertos/semphr.h"

#define SERVO_MIN_PULSEWIDTH_US 500  
#define SERVO_MAX_PULSEWIDTH_US 2500 
#define SERVO_MIN_DEGREE -180
#define SERVO_MAX_DEGREE 180

#define LEDC_MAX_DUTY 16383
#define SERVO_PERIOD_US 20000

static SemaphoreHandle_t servo_mutex = NULL;

static const int servo_pins[] = {SERVO0_PWM, SERVO1_PWM, SERVO2_PWM, SERVO3_PWM};

namespace Servo {

void init() {
  if (servo_mutex == NULL) {
    servo_mutex = xSemaphoreCreateMutex();
  }

  for (int i = 0; i < 4; i++) {
    // 1. Configure the LEDC channel (Channel 0-3, 50 Hz, 14-bit resolution)
    ledcSetup(i, 50, 14);
    
    // 2. Attach the physical GPIO pin to that channel
    ledcAttachPin(servo_pins[i], i);
  }
}

void write(SERVO s, int16_t angle) {
  if (servo_mutex == NULL) return;

  if (xSemaphoreTake(servo_mutex, portMAX_DELAY) == pdTRUE) {

    angle = constrain(angle, SERVO_MIN_DEGREE, SERVO_MAX_DEGREE);

    uint32_t pulse_us = map(angle, SERVO_MIN_DEGREE, SERVO_MAX_DEGREE, 
                            SERVO_MIN_PULSEWIDTH_US, SERVO_MAX_PULSEWIDTH_US);
                            
    uint32_t duty = map(pulse_us, 0, SERVO_PERIOD_US, 0, LEDC_MAX_DUTY);

    // Cast enum to integer to get the corresponding channel (0, 1, 2, or 3)
    int channel = static_cast<int>(s);
    
    // 3. Write the duty cycle to the channel (Core v2 uses channel, not pin)
    ledcWrite(channel, duty);

    vTaskDelay(pdMS_TO_TICKS(50));
    xSemaphoreGive(servo_mutex);
  }
}

} // namespace Servo