/**
 * @file  dc_motor.h
 * @brief dc driver (in PHASE/ENABLE mode)
 *
 */

#pragma once

namespace DCMotor {
  /**
   * @brief DC motor enum
   */
  enum DCMOTOR {
    MOTOR_0, MOTOR_1, MOTOR_2, MOTOR_3
  };

  /**
   * @brief Initialize all motors 
   */
  void init();

   /**
   * @brief Drive a certain motor at a certain speed (-100, 100)
   */
  void write(DCMOTOR d, int16_t speed);
}