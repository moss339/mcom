#ifndef MSERVICE_ERROR_H
#define MSERVICE_ERROR_H

#include <cstdint>

namespace mservice {

enum class ServiceError : uint8_t {
    OK = 0x00,
    TIMEOUT = 0x01,
    SERVICE_NOT_FOUND = 0x02,
    METHOD_NOT_FOUND = 0x03,
    INVALID_REQUEST = 0x04,
    INTERNAL_ERROR = 0x05,
    CANCELED = 0x06,
};

inline const char* service_error_to_string(ServiceError error) {
    switch (error) {
        case ServiceError::OK: return "OK";
        case ServiceError::TIMEOUT: return "TIMEOUT";
        case ServiceError::SERVICE_NOT_FOUND: return "SERVICE_NOT_FOUND";
        case ServiceError::METHOD_NOT_FOUND: return "METHOD_NOT_FOUND";
        case ServiceError::INVALID_REQUEST: return "INVALID_REQUEST";
        case ServiceError::INTERNAL_ERROR: return "INTERNAL_ERROR";
        case ServiceError::CANCELED: return "CANCELED";
        default: return "UNKNOWN";
    }
}

}  // namespace mservice

#endif  // MSERVICE_ERROR_H
