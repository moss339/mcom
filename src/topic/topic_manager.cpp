#include "mcom/topic/topic_manager.h"

namespace mcom {
namespace topic {

TopicManager& TopicManager::instance() {
    static TopicManager instance;
    return instance;
}

} // namespace topic
} // namespace mcom
