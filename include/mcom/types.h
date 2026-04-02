/**
 * @file types.h
 * @brief Common type definitions for MCOM module
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <optional>

namespace moss {
namespace mcom {

/**
 * @brief MCOM Type Definitions
 * @{
 */

/** @brief Service identifier type */
using ServiceId = uint16_t;

/** @brief Service instance identifier type */
using InstanceId = uint16_t;

/** @brief Method identifier type */
using MethodId = uint16_t;

/** @brief Event identifier type */
using EventId = uint16_t;

/** @brief Action identifier type */
using ActionId = uint16_t;

/** @} */

/**
 * @brief MCOM Error Codes
 *
 * Standard error codes used across MCOM for error handling.
 */
enum class McomError : uint8_t {
    OK = 0x00,             /**< @brief Success */
    TIMEOUT = 0x01,        /**< @brief Operation timed out */
    NOT_FOUND = 0x02,      /**< @brief Resource not found */
    INVALID_REQUEST = 0x03, /**< @brief Invalid request format */
    INTERNAL_ERROR = 0x04,  /**< @brief Internal error */
    CANCELED = 0x05,        /**< @brief Operation was canceled */
};

}  // namespace mcom
}  // namespace moss
