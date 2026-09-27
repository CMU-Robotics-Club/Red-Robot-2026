#include "battery.h"
#include "pinouts.h"
#include <Arduino.h>

namespace Battery {
float read() {
  // 12-bit resolution - experimentally calculated ADC inaccuracy to get 3770
  // and Vout = (R27/(R26+R27))Vin
  // R26 = 1M, R27 = 100k
  // so Vin = Vout * ((R26+R27)/R27) = (3.3*(ADC/resolution)) * 11
  return 11 * 3.3 * (analogRead(ADC1_0) / 3770.0);
}
} // namespace BatterVproty