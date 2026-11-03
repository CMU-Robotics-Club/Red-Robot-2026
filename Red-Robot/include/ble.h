/**
 * @file  ble.h
 * @brief BLE driver
 *
 */

#pragma once

#define BLE_STATIC_PIN 232323

extern volatile int16_t global_motor_speed[4];

namespace BLE {
/**
 * @brief Initialize BLE stack and start advertising
 */
void init();
}