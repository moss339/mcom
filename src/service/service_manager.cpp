#include "mservice/service_manager.h"

namespace mservice {

ServiceManager& ServiceManager::instance() {
    static ServiceManager instance;
    return instance;
}

SessionId ServiceManager::generate_session_id() {
    return next_session_id_++;
}

}  // namespace mservice
