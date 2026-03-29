#ifndef MCOM_NODE_CONFIG_H
#define MCOM_NODE_CONFIG_H

#include <cstdint>
#include <string>
#include <vector>

namespace mcom {

struct NodeConfig {
    std::string node_name;
    uint8_t domain_id{0};
    bool enable_multicast_discovery{false};

    NodeConfig() = default;

    explicit NodeConfig(const std::string& name, uint8_t domain = 0)
        : node_name(name), domain_id(domain) {}
};

}  // namespace mcom

#endif  // MCOM_NODE_CONFIG_H
