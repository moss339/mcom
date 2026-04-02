/**
 * @file mcom.h
 * @brief MCOM - Unified Communication Middleware
 *
 * MCOM (Middleware for Object-Oriented Shared Services) provides a unified
 * communication layer for automotive and embedded systems. It combines
 * Topic publish/subscribe, Service RPC, and Action patterns.
 *
 * @section architecture Architecture
 *
 * MCOM is built in layers:
 * - topic/: Publish/subscribe layer (wraps mdds)
 * - service/: RPC service layer (based on protobuf)
 * - action/: Action layer (based on protobuf)
 * - node/: Unified entry point for users
 *
 * @section usage Usage Example
 *
 * @code
 * // Create a node
 * auto node = moss::mcom::Node::create("my_node");
 *
 * // Create publisher for Image topic
 * auto pub = node->create_publisher<moss::proto::topic::Image>("/camera/image");
 *
 * // Create subscriber for LaserScan topic
 * auto sub = node->create_subscriber<moss::proto::topic::LaserScan>("/scan",
 *     [](const auto& msg) {
 *         // Handle received scan
 *     });
 *
 * // Create service client
 * auto client = node->create_proto_service_client<ExampleService>("example");
 *
 * // Spin to process callbacks
 * node->spin();
 * @endcode
 */

/**
 * @mainpage MCOM API Documentation
 *
 * @section overview Overview
 *
 * MCOM is a lightweight middleware providing:
 * - Topic-based publish/subscribe (moss::mcom::topic::ProtoPublisher/ProtoSubscriber)
 * - RPC services (moss::mcom::service::ProtoServiceClient/ProtoServiceServer)
 * - Long-running actions (moss::mcom::action::ProtoActionClient/ProtoActionServer)
 *
 * @section dependencies Dependencies
 *
 * MCOM depends on:
 * - mdds: Low-level DDS implementation for transport
 * - proto: Protocol buffer message definitions
 * - mruntime: Runtime foundation (Node lifecycle)
 */

#pragma once
#include "types.h"
#include "topic/publisher.h"
#include "topic/subscriber.h"
#include "topic/topic_manager.h"
#include "service/service_client.h"
#include "service/service_server.h"
#include "action/action_client.h"
#include "action/action_server.h"
#include "action/goal_handle.h"
#include "node/node.h"

namespace moss {
namespace mcom {

/** @brief MCOM version string */
constexpr const char* VERSION = "0.1.0";

}  // namespace mcom
}  // namespace moss
