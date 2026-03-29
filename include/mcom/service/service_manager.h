#ifndef MSERVICE_SERVICE_MANAGER_H
#define MSERVICE_SERVICE_MANAGER_H

#include "types.h"
#include <map>
#include <mutex>
#include <shared_mutex>

namespace mservice {

class ServiceManager {
public:
    static ServiceManager& instance();

    SessionId generate_session_id();

private:
    ServiceManager() : next_session_id_(1) {}

    SessionId next_session_id_;
};

}  // namespace mservice

#endif  // MSERVICE_SERVICE_MANAGER_H
