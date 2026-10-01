/**
 * @file  ble.h
 * @brief BLE driver
 *
 */

#pragma once

extern volatile int16_t global_motor_speed;

namespace BLE {
void init();
}