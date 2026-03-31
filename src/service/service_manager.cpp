#include "mcom/service/service_manager.h"

namespace moss {
namespace mcom {
namespace service {

ServiceManager& ServiceManager::instance() {
    static ServiceManager instance;
    return instance;
}

SessionId ServiceManager::generate_session_id() {
    return next_session_id_++;
}

}  // namespace service
}  // namespace mcom

}  // namespace moss
