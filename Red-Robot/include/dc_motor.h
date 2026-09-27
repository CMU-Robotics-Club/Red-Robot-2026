/**
 * @file  dc_motor.h
 * @brief dc driver (in PHASE/ENABLE mode)
 *
 */

#pragma once

#include <stdint.h>

namespace DCMotor {
  /**
   * @brief DC motor enum
   */
  enum class DCMOTOR {
    MOTOR_0, MOTOR_1, MOTOR_2, MOTOR_3
  };

  enum class DIRECTION {
    FORWARD = 0, BACKWARD = 1
  };


  /**
   * @brief Initialize all motors 
   */
  void init();

   /**
   * @brief Drive a certain motor at a certain duty cycle (-100 to 100)
   * 
   */
  void write(DCMOTOR d, int16_t duty_cycle);
}