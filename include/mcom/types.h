#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <optional>

namespace mcom {

using ServiceId = uint16_t;
using InstanceId = uint16_t;
using MethodId = uint16_t;
using EventId = uint16_t;
using ActionId = uint16_t;

enum class McomError : uint8_t {
    OK = 0x00,
    TIMEOUT = 0x01,
    NOT_FOUND = 0x02,
    INVALID_REQUEST = 0x03,
    INTERNAL_ERROR = 0x04,
    CANCELED = 0x05,
};

} // namespace mcom
