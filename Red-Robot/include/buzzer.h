/**
 * @file  buzzer.h
 * @brief Buzzer PWM driver
 *
 */

#pragma once

namespace Buzzer {
  /**
   * @brief Initialize buzzer PWM
   */
  void init();

  /**
   * @brief Play startup sound
   */
  void play_startup();
}