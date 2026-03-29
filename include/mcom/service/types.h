#ifndef MSERVICE_TYPES_H
#define MSERVICE_TYPES_H

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <future>

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

using ServiceId = uint16_t;
using InstanceId = uint16_t;
using MethodId = uint16_t;
using SessionId = uint16_t;

struct RequestHeader {
    SessionId session_id;
    ServiceId service_id;
    InstanceId instance_id;
    MethodId method_id;
};

struct ResponseHeader {
    SessionId session_id;
    ServiceId service_id;
    InstanceId instance_id;
    MethodId method_id;
};

struct Request {
    RequestHeader header;
    std::vector<uint8_t> payload;
};

struct Response {
    ResponseHeader header;
    ServiceError error;
    std::vector<uint8_t> payload;
};

using RequestHandler = std::function<Response(const Request& request)>;

}  // namespace mservice

#endif  // MSERVICE_TYPES_H
