/**
 * @file  line_sensors.h
 * @brief Line sensor interface
 *
 */

#pragma once

#include <stdint.h>

namespace LineSensors {
  /**
   * @brief Initialize line sensors
   */
  void init();

  /**
   * @brief Read line sensor values
   *
   * Reads the values from the 6 line sensors and stores them in the provided
   * array. The values represent the time taken for each sensor to discharge,
   * which can be used to determine the presence of a line.
   *
   * @param values An array of 6 integers to store the sensor readings.
   */
  void read(uint16_t values[6]);
}