# MCom 模块设计文档

## 1. 概述

MCom 是 MOSS 项目的统一通信中间件，封装了 Topic（发布/订阅）、Service（RPC）、Action（动作）三种通信模式，提供 Node 统一入口。

MCom 依赖外部 `mdds`（DDS 底层）和 `mshm`（共享内存），自身仅做封装和上层实现。

## 2. 架构层次

```
mdds (外部依赖，DDS Topic 底层)
    ↑
mcom/topic   (封装 mdds，提供 Topic 接口)
    ↑
mcom/service (基于 topic 实现 RPC Request/Response)
    ↑
mcom/action  (基于 service 实现 Goal/Result/Feedback)
    ↑
mcom/node    (统一封装，用户入口)
```

## 3. 目录结构

```
mcom/
├── include/mcom/
│   ├── mcom.h              # 主头文件
│   ├── types.h             # 公共类型定义
│   ├── topic/             # Topic 层
│   │   ├── publisher.h
│   │   ├── subscriber.h
│   │   └── topic_manager.h
│   ├── service/           # Service 层
│   │   ├── service_client.h
│   │   ├── service_server.h
│   │   └── service_manager.h
│   ├── action/            # Action 层
│   │   ├── action_client.h
│   │   ├── action_server.h
│   │   ├── action_manager.h
│   │   └── goal_handle.h
│   └── node/             # Node 统一封装
│       └── node.h
├── src/
│   ├── topic/
│   ├── service/
│   ├── action/
│   └── node/
├── examples/
├── test/
└── CMakeLists.txt
```

## 4. Topic 层

### 4.1 职责
封装 `mdds` 的 DDS Topic 接口，提供统一的发布/订阅 API。

### 4.2 核心类

```cpp
namespace mcom::topic {

class Publisher {
public:
    virtual ~Publisher() = default;
    virtual void publish(const void* data, size_t size) = 0;
    virtual void publish(const void* data, size_t size, uint64_t timestamp) = 0;
    const std::string& get_topic_name() const;
};

class Subscriber {
public:
    virtual ~Subscriber() = default;
    virtual void subscribe() = 0;
    virtual void unsubscribe() = 0;
    const std::string& get_topic_name() const;
};

class TopicManager {
public:
    PublisherPtr create_publisher(const std::string& topic_name);
    SubscriberPtr create_subscriber(const std::string& topic_name, 
                                    MessageCallback callback);
};

using MessageCallback = std::function<void(const void* data, size_t size)>;

}
```

## 5. Service 层

### 5.1 职责
基于 Topic 实现 RPC 风格的 Request/Response 通信，支持同步/异步调用。

### 5.2 核心类

```cpp
namespace mcom::service {

enum class ServiceError : uint8_t {
    OK = 0x00,
    TIMEOUT = 0x01,
    NOT_FOUND = 0x02,
    METHOD_NOT_FOUND = 0x03,
    INVALID_REQUEST = 0x04,
    INTERNAL_ERROR = 0x05,
    CANCELED = 0x06,
};

struct Request {
    // header + payload
};

struct Response {
    // header + payload + error
};

using ServiceHandler = std::function<Response(const Request& request)>;

class ServiceClient {
public:
    std::optional<Response> send_request(MethodId method_id,
                                         const std::vector<uint8_t>& payload,
                                         std::chrono::milliseconds timeout);
    std::future<std::optional<Response>> send_request_async(...);
    bool send_request_no_return(MethodId method_id, const std::vector<uint8_t>& payload);
};

class ServiceServer {
public:
    void register_method(MethodId method_id, ServiceHandler handler);
    void offer();
    void stop_offer();
};

}
```

### 5.3 SOME/IP 映射

| 操作 | SOME/IP MessageType |
|------|---------------------|
| Request | REQUEST (0x00) |
| RequestNoReturn | REQUEST_NO_RETURN (0x01) |
| Response | RESPONSE (0x80) |
| Error | ERROR (0x81) |

## 6. Action 层

### 6.1 职责
基于 Service 实现长时间运行任务的 Goal/Result/Feedback 机制，支持取消和进度跟踪。

### 6.2 核心类

```cpp
namespace mcom::action {

enum class ActionStatus : uint8_t {
    READY = 0x00,
    EXECUTING = 0x01,
    PREEMPTING = 0x02,
    PREEMPTED = 0x03,
    SUCCEEDING = 0x04,
    SUCCEEDED = 0x05,
    ABORTING = 0x06,
    ABORTED = 0x07,
    CANCELED = 0x08,
    REJECTED = 0x09,
};

class GoalHandle {
public:
    uint32_t get_goal_id() const;
    ActionStatus get_status() const;
    bool is_terminal() const;
    bool cancel_requested() const;
    void request_cancel();
};

class ActionClient {
public:
    std::shared_ptr<GoalHandle> send_goal(const std::vector<uint8_t>& goal_data);
    bool cancel_goal(uint32_t goal_id);
    void subscribe_feedback(FeedbackHandler handler);
};

class ActionServer {
public:
    void register_goal_handler(GoalHandler handler);
    void accept_goal(uint32_t goal_id);
    void reject_goal(uint32_t goal_id);
    void publish_feedback(uint32_t goal_id, float progress);
    void send_result(uint32_t goal_id, ActionStatus status);
};

}
```

### 6.3 状态机

```
READY → EXECUTING → SUCCEEDED
                  → ABORTED
                  → CANCELED
```

## 7. Node 层

### 7.1 职责
统一封装 Topic、Service、Action 接口，作为用户的统一入口。

### 7.2 Node 核心 API

```cpp
namespace mcom::node {

class Node {
public:
    explicit Node(const std::string& name);
    ~Node();
    
    // Topic
    topic::PublisherPtr create_publisher(const std::string& topic_name);
    topic::SubscriberPtr create_subscriber(const std::string& topic_name,
                                           topic::MessageCallback callback);
    
    // Service
    service::ServiceClientPtr create_service_client(const std::string& service_name);
    service::ServiceServerPtr create_service_server(const std::string& service_name,
                                                     service::ServiceHandler handler);
    
    // Action
    action::ActionClientPtr create_action_client(const std::string& action_name);
    action::ActionServerPtr create_action_server(const std::string& action_name,
                                                 action::ActionHandler handler);
    
    const std::string& get_name() const;
    void spin();
};

}
```

### 7.3 使用示例

```cpp
#include "mcom/mcom.h"

int main() {
    auto node = mcom::node::Node::create("perception_node");
    
    // Topic
    auto pub = node->create_publisher("/camera/image");
    pub->publish(image_data, size);
    
    auto sub = node->create_subscriber("/lidar/points",
        [](const void* data, size_t size) {
            // handle point cloud
        });
    
    // Service
    auto client = node->create_service_client("/detection");
    auto response = client->send_request(0x0001, payload);
    
    // Action
    auto action_client = node->create_action_client("/navigate");
    auto goal_handle = action_client->send_goal(goal_data);
    
    node->spin();
    return 0;
}
```

## 8. 依赖关系

```
mcom
├── mshm (共享内存)
└── mdds (DDS Topic)
```

## 9. 构建配置

```bash
cd mcom && mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```
