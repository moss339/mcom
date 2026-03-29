#ifndef MSERVICE_SERVICE_CLIENT_H
#define MSERVICE_SERVICE_CLIENT_H

#include "types.h"
#include <memory>
#include <chrono>
#include <future>

namespace mservice {

class ServiceClient : public std::enable_shared_from_this<ServiceClient> {
public:
    ServiceClient(ServiceId service_id, InstanceId instance_id);
    ~ServiceClient();

    void init();

    std::optional<Response> send_request(
        MethodId method_id,
        const std::vector<uint8_t>& payload,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    std::future<std::optional<Response>> send_request_async(
        MethodId method_id,
        const std::vector<uint8_t>& payload,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    bool send_request_no_return(MethodId method_id,
                                const std::vector<uint8_t>& payload);

    bool is_service_available() const;

    ServiceId get_service_id() const;
    InstanceId get_instance_id() const;

private:
    ServiceId service_id_;
    InstanceId instance_id_;
    bool initialized_;
};

}  // namespace mservice

#endif  // MSERVICE_SERVICE_CLIENT_H
