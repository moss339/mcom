# MOSS Communication Middleware (mcom)

MOSS unified communication middleware providing Topic publish/subscribe, Service RPC, and Action communication patterns based on Protocol Buffers.

## Features

- **Topic Layer**: ProtoPublisher/ProtoSubscriber for publish-subscribe communication
- **Service Layer**: ProtoServiceClient/ProtoServiceServer for request-response RPC
- **Action Layer**: ProtoActionClient/ProtoActionServer for long-running actions with feedback
- **Node**: Unified entry point for creating publishers, subscribers, and clients
- **C++17**: Modern C++ with smart pointers and template support

## Architecture

```
mcom/
├── include/mcom/
│   ├── mcom.h              # Main include
│   ├── types.h             # Common types
│   ├── topic/              # Publish/subscribe layer
│   │   ├── publisher.h
│   │   ├── subscriber.h
│   │   ├── proto_publisher.h
│   │   └── proto_subscriber.h
│   ├── service/            # RPC service layer
│   │   ├── service_client.h
│   │   ├── service_server.h
│   │   ├── proto_service_client.h
│   │   └── proto_service_server.h
│   ├── action/             # Action layer
│   │   ├── action_client.h
│   │   ├── action_server.h
│   │   ├── proto_action_client.h
│   │   └── proto_action_server.h
│   └── node/               # User entry point
│       └── node.h
└── src/                    # Implementation
```

## Dependencies

- mdds (Data Distribution Service)
- mshm (Shared Memory)
- mruntime (Runtime foundation)
- proto (Protocol Buffers message definitions)

## Building

```bash
cd mcom
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

## Usage Example

```cpp
#include <mcom/mcom.h>
#include <mcom/node/node.h>

int main() {
    // Create a node
    auto node = moss::mcom::Node::create("my_node", 0);

    if (!node->init()) {
        return -1;
    }

    // Create a publisher for Image messages
    auto publisher = node->create_publisher<proto::topic::Image>("camera/image");

    // Create a subscriber for PointCloud messages
    auto subscriber = node->create_subscriber<proto::topic::PointCloud2>(
        "lidar/points",
        [](const proto::topic::PointCloud2& data) {
            // Handle received data
        });

    // Create a service client
    auto service_client = node->create_proto_service_client("navigation", 0x1001);

    // Create an action client
    auto action_client = node->create_action_client<proto::action::MoveGoal>("move_base", 0x2001);

    node->start();
    node->spin();

    return 0;
}
```

## Node API

The `Node` class provides a unified interface:

- `create_publisher<T>(topic_name)` - Create a ProtoPublisher
- `create_subscriber<T>(topic_name, callback)` - Create a ProtoSubscriber
- `create_service_client(service_name, service_id)` - Create a ProtoServiceClient
- `create_service_server(service_name, service_id)` - Create a ProtoServiceServer
- `create_action_client<T>(action_name, action_id)` - Create a ProtoActionClient
- `create_action_server<T, R>(action_name, action_id)` - Create a ProtoActionServer

## License

MIT License
