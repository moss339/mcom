#ifndef MSERVICE_SERVICE_SERVER_H
#define MSERVICE_SERVICE_SERVER_H

#include "types.h"
#include <memory>
#include <map>

namespace mservice {

class ServiceServer : public std::enable_shared_from_this<ServiceServer> {
public:
    ServiceServer(ServiceId service_id, InstanceId instance_id);
    ~ServiceServer();

    void init();
    void offer();
    void stop_offer();

    void register_method(MethodId method_id, RequestHandler handler);
    void unregister_method(MethodId method_id);

    void send_response(const Request& request, const std::vector<uint8_t>& payload);
    void send_error_response(const Request& request, ServiceError error);

    ServiceId get_service_id() const;
    InstanceId get_instance_id() const;

private:
    ServiceId service_id_;
    InstanceId instance_id_;
    std::map<MethodId, RequestHandler> handlers_;
    bool offered_;
};

}  // namespace mservice

#endif  // MSERVICE_SERVICE_SERVER_H
