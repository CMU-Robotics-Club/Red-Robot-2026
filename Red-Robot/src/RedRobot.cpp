#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include "driver/mcpwm.h"
#include "freertos/semphr.h"

#include "RedRobot.h"
#include "pinouts.h"
#include "ble.h"

#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_MIN_DEGREE -180
#define SERVO_MAX_DEGREE 180

#define SERVO_LEDC_MAX_DUTY 16383
#define SERVO_PERIOD_US 20000

#define MOTOR_PWM_FREQ 20000

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
  enum DCMOTOR {
    MOTOR_0, MOTOR_1, MOTOR_2, MOTOR_3
  };


void init() {
  if (dc_mutex == NULL) {
    dc_mutex = xSemaphoreCreateMutex();
  }

  Serial.println("[DC_MOTOR] Initializing channels...");

  for (int i = 0; i < 4; i++) {
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

  int motor = static_cast<int>(d);

  if (motor < 0 || motor >= 4) {
    Serial.printf("[DC_MOTOR] Error: Invalid motor index %d\n", motor);
    return;
  }

  if (xSemaphoreTake(dc_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    Serial.println("[DC_MOTOR] Error: Mutex lock timeout!");
    return;
  }

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
}

namespace {
static SemaphoreHandle_t servo_mutex = NULL;

static const int servo_pins[] = {SERVO0_PWM, SERVO1_PWM, SERVO2_PWM,
                                 SERVO3_PWM};
} // namespace

namespace Servo {
  enum SERVO{
    SERVO_0, SERVO_1, SERVO_2, SERVO_3
  };

void init() {
  if (servo_mutex == NULL) {
    servo_mutex = xSemaphoreCreateMutex();
  }
  ledcSetClockSource(LEDC_USE_XTAL_CLK);

  for (int i = 0; i < 4; i++) {
    // (Channel 0-3, 50 Hz, 14-bit resolution)
    ledcAttach(servo_pins[i], 50, 14);
  }
}

void write(SERVO s, int16_t angle) {
  if (servo_mutex == NULL)
    return;

  if (xSemaphoreTake(servo_mutex, portMAX_DELAY) == pdTRUE) {

    angle = constrain(angle, SERVO_MIN_DEGREE, SERVO_MAX_DEGREE);

    uint32_t pulse_us = map(angle, SERVO_MIN_DEGREE, SERVO_MAX_DEGREE,
                            SERVO_MIN_PULSEWIDTH_US, SERVO_MAX_PULSEWIDTH_US);

    uint32_t duty = map(pulse_us, 0, SERVO_PERIOD_US, 0, SERVO_LEDC_MAX_DUTY);

    // Cast enum to integer to get the corresponding channel (0, 1, 2, or 3)
    int idx = static_cast<int>(s);

    // 3. Write the duty cycle to the channel (Core v2 uses channel, not pin)
    ledcWrite(servo_pins[idx], duty);   // pin, not channel
    xSemaphoreGive(servo_mutex);
  }
}

} // namespace Servo

namespace {
/**
 * @brief SPI driver for MAX7219EWG
 *
 * This should be thread safe since only this chip uses the bus
 */
SPIClass spi(HSPI);

/**
 * @brief SPI write
 */
void spi_write(uint8_t address, uint8_t data) {
  // 10MHz clock speed, MSB First, SPI Mode 0
  spi.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  digitalWrite(BMS_LOAD, LOW);  // Select MAX7219
  spi.transfer(address);        // Transmit address
  spi.transfer(data);           // Transmit data
  digitalWrite(BMS_LOAD, HIGH); // Latch data into MAX7219
  spi.endTransaction();
}

/**
 * @brief Draw a single digit (0 to 9) at a position in [3,2,1]
 */
void show_digit(uint8_t position, uint8_t number, bool decimal_point) {
  if (position >= 1 && position <= 3 && number <= 9) {
    number |= decimal_point ? 0x80 : 0;
    spi_write(position, number);
  }
}
} // namespace

/**
 * @brief Sends packets to configure the display's brightness, number of digits, etc.
 */
void setup_display() {
  spi_write(0x09, 0x07); // decode mode: enable Code B decode for digits 0-2 to write digits directly
  spi_write(0x0A, 0x0F); // intensity: set to max brightness
  spi_write(0x0B, 0x02); // scan limit: display only 3 digits
  spi_write(0x0C, 0x01); // shutdown: turn on normal operation
  spi_write(0x0F, 0x00); // display Test off
}

namespace Display {
void show(uint16_t value) {
  // Clamp from 0 to 999
  if (value > 999)
    value = 999;
  uint8_t digit1 = value / 100;
  uint8_t digit2 = (value / 10) % 10;
  uint8_t digit3 = value % 10;
  //setup_display();
  show_digit(1, digit1, true); // decimal point
  show_digit(2, digit2, false);
  show_digit(3, digit3, false);
}

void init() {
  pinMode(BMS_LOAD, OUTPUT);
  digitalWrite(BMS_LOAD, HIGH);
  // no MISO since the driver chip doesn't talk back
  spi.begin(BMS_CLK, -1, BMS_SDI, BMS_LOAD);
  setup_display();
  show(0);               // initialize with 0.00
}
} // namespace Display

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