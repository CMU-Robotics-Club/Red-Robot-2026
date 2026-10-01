/**
 * @file  ble.h
 * @brief BLE driver
 *
 */
#pragma once

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

extern volatile int16_t global_motor_speed = 0;

namespace BLE {
void init();
}