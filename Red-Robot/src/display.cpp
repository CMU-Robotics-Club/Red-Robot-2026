#include <Arduino.h>

#include "display.h"
#include "pinouts.h"
#include <SPI.h>
#include <stdint.h>

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
void init() {
  pinMode(BMS_LOAD, OUTPUT);
  digitalWrite(BMS_LOAD, HIGH);
  // no MISO since the driver chip doesn't talk back
  spi.begin(BMS_CLK, -1, BMS_SDI, BMS_LOAD);
  setup_display();
  show(0);               // initialize with 0.00
}

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
} // namespace Display