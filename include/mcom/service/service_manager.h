#ifndef MCOM_SERVICE_MANAGER_H
#define MCOM_SERVICE_MANAGER_H

#include "types.h"
#include <map>
#include <mutex>
#include <shared_mutex>

namespace moss {
namespace mcom {
namespace service {

class ServiceManager {
public:
    static ServiceManager& instance();

    SessionId generate_session_id();

private:
    ServiceManager() : next_session_id_(1) {}

    SessionId next_session_id_;
};

}  // namespace service
}  // namespace mcom

}  // namespace moss
#endif  // MCOM_SERVICE_MANAGER_H
