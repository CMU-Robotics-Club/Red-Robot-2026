#include <Arduino.h>

#include "battery.h"
#include "pinouts.h"

namespace Battery {
float read() {
  // R26 = 100k, R27 = 33k
  // so Vin = Vout * ((R26+R27)/R27) = Vout * 4.0303
  uint32_t mv = analogReadMilliVolts(ADC1_0);
  float volts = mv / 1000.0; 
  return volts * 4.0303;
}
} // namespace Battery