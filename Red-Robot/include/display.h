/**
 * @file  display.h
 * @brief 7-segment display interface
 *
 */

#pragma once

#include <stdint.h>

namespace Display {
/**
 * @brief Initializes the display driver
 *
 * Starts the background task that multiplexes the 3-digit battery monitor
 * display. Call once from setup(). Display shows "00.0" until the first
 * displayNumber() call.
 */
void init();

/**
 * @brief Displays a number from 0-999 in hundredths
 *
 * Shows `value` (0-999) as "X.XX" (i.e. value is the number in hundredths). Values
 * above 999 are clamped to 999.
 */
void show(uint16_t value);
} // namespace Display
