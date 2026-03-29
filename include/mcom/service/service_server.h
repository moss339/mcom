#ifndef MCOM_SERVICE_SERVER_H
#define MCOM_SERVICE_SERVER_H

#include "types.h"
#include <memory>
#include <map>
#include <mutex>
#include "mcom/topic/topic_manager.h"

namespace mcom {
namespace service {

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
    std::string get_request_topic() const;
    std::string get_response_topic() const;
    void handle_request(const Request& request);

    ServiceId service_id_;
    InstanceId instance_id_;
    std::map<MethodId, RequestHandler> handlers_;
    bool offered_;
    std::mutex handlers_mutex_;
};

using ServiceServerPtr = std::shared_ptr<ServiceServer>;

}  // namespace service
}  // namespace mcom

#endif  // MCOM_SERVICE_SERVER_H
