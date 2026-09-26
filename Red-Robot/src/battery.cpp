#include "battery.h"
#include "pinouts.h"
#include <Arduino.h>

namespace Battery {
float read() {
  // 12-bit resolution
  // and Vout = (R27/(R26+R27))Vin
  // R26 = 1M, R27 = 100k
  // so Vin = Vout * ((R26+R27)/R27) = (3.3*(ADC/resolution)) * 11
  return 11 * 3.3 * (analogRead(ADC1_0) / 4096.0);
}
} // namespace Battery