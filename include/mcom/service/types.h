#ifndef MCOM_SERVICE_TYPES_H
#define MCOM_SERVICE_TYPES_H

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <future>
#include <string>
#include <cstring>

namespace mcom {
namespace service {

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

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        size_t total_size = sizeof(RequestHeader) + sizeof(size_t) + payload.size();
        buffer.resize(total_size);
        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &header, sizeof(RequestHeader));
        offset += sizeof(RequestHeader);
        size_t payload_size = payload.size();
        std::memcpy(buffer.data() + offset, &payload_size, sizeof(size_t));
        offset += sizeof(size_t);
        if (!payload.empty()) {
            std::memcpy(buffer.data() + offset, payload.data(), payload.size());
        }
        return buffer;
    }

    static Request deserialize(const uint8_t* buffer, size_t size) {
        Request req;
        size_t offset = 0;
        if (offset + sizeof(RequestHeader) > size) return req;
        std::memcpy(&req.header, buffer + offset, sizeof(RequestHeader));
        offset += sizeof(RequestHeader);
        if (offset + sizeof(size_t) > size) return req;
        size_t payload_size;
        std::memcpy(&payload_size, buffer + offset, sizeof(size_t));
        offset += sizeof(size_t);
        if (offset + payload_size > size) return req;
        req.payload.resize(payload_size);
        if (payload_size > 0) {
            std::memcpy(req.payload.data(), buffer + offset, payload_size);
        }
        return req;
    }
};

struct Response {
    ResponseHeader header;
    ServiceError error;
    std::vector<uint8_t> payload;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        size_t total_size = sizeof(ResponseHeader) + sizeof(ServiceError) + sizeof(size_t) + payload.size();
        buffer.resize(total_size);
        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &header, sizeof(ResponseHeader));
        offset += sizeof(ResponseHeader);
        std::memcpy(buffer.data() + offset, &error, sizeof(ServiceError));
        offset += sizeof(ServiceError);
        size_t payload_size = payload.size();
        std::memcpy(buffer.data() + offset, &payload_size, sizeof(size_t));
        offset += sizeof(size_t);
        if (!payload.empty()) {
            std::memcpy(buffer.data() + offset, payload.data(), payload.size());
        }
        return buffer;
    }

    static Response deserialize(const uint8_t* buffer, size_t size) {
        Response resp;
        size_t offset = 0;
        if (offset + sizeof(ResponseHeader) > size) return resp;
        std::memcpy(&resp.header, buffer + offset, sizeof(ResponseHeader));
        offset += sizeof(ResponseHeader);
        if (offset + sizeof(ServiceError) > size) return resp;
        std::memcpy(&resp.error, buffer + offset, sizeof(ServiceError));
        offset += sizeof(ServiceError);
        if (offset + sizeof(size_t) > size) return resp;
        size_t payload_size;
        std::memcpy(&payload_size, buffer + offset, sizeof(size_t));
        offset += sizeof(size_t);
        if (offset + payload_size > size) return resp;
        resp.payload.resize(payload_size);
        if (payload_size > 0) {
            std::memcpy(resp.payload.data(), buffer + offset, payload_size);
        }
        return resp;
    }
};

using RequestHandler = std::function<Response(const Request& request)>;

struct ServiceMessage {
    std::vector<uint8_t> serialize() const {
        return data_;
    }

    static ServiceMessage deserialize(const uint8_t* buffer, size_t size) {
        ServiceMessage msg;
        msg.data_.resize(size);
        std::memcpy(msg.data_.data(), buffer, size);
        return msg;
    }

    std::vector<uint8_t> data_;
};

}  // namespace service
}  // namespace mcom

#endif  // MCOM_SERVICE_TYPES_H
