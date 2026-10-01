/**
 * @file  servo.h
 * @brief Servo driver
 *
 */

#pragma once

#include <stdint.h>

namespace Servo {
  /**
   * @brief Servo enum
   */
  enum SERVO{
    SERVO_0, SERVO_1, SERVO_2, SERVO_3
  };

  /**
   * @brief Initialize all servos
   */
  void init();

  /**
   * @brief Drive a certain servo to a specific angle
   * 
   * Angle from -180 to 180
   */
  void write(SERVO s, int16_t angle);
}