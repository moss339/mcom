#ifndef MCOM_NODE_STATE_H
#define MCOM_NODE_STATE_H

#include <stdexcept>
#include <string>

namespace mcom {

enum class NodeState {
    UNINITIALIZED = 0,
    INITIALIZED = 1,
    RUNNING = 2,
    STOPPED = 3,
    DESTROYED = 4
};

class NodeException : public std::runtime_error {
public:
    explicit NodeException(const std::string& message) : std::runtime_error(message) {}
};

class NodeStateException : public NodeException {
public:
    NodeStateException(NodeState current, NodeState expected);
    NodeState get_current_state() const { return current_; }
    NodeState get_expected_state() const { return expected_; }

private:
    NodeState current_;
    NodeState expected_;
};

inline const char* to_string(NodeState state) {
    switch (state) {
        case NodeState::UNINITIALIZED: return "UNINITIALIZED";
        case NodeState::INITIALIZED:   return "INITIALIZED";
        case NodeState::RUNNING:       return "RUNNING";
        case NodeState::STOPPED:       return "STOPPED";
        case NodeState::DESTROYED:     return "DESTROYED";
        default:                        return "UNKNOWN";
    }
}

}  // namespace mcom

#endif  // MCOM_NODE_STATE_H
