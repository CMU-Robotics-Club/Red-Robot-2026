/**
 * @file  battery.h
 * @brief Battery level reporting
 *
 */

#pragma once

#include <stdint.h>

namespace Battery {
/**
 * @brief Read the battery level
 */
float read();
}