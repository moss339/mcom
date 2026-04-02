# MOSS mcom 迁移指南

> 版本：v1.0
> 日期：2026-04-02

---

## 1. 概述

本文档指导开发者从 mcom 旧 API 迁移到新 API。

### 1.1 迁移背景

mcom v2.0 进行了 Protobuf 重构，主要改动：

- **mdds 层**：新增 `DataWriterRaw`/`DataReaderRaw`，不依赖类型序列化
- **mcom 层**：`ProtoPublisher<T>`/`ProtoSubscriber<T>` 封装 Raw 接口，使用 protobuf 序列化
- **旧 API**：`DataWriter<T>`/`DataReader<T>` 依赖 `T::serialize()`/`T::deserialize()`，已标记为 deprecated

### 1.2 迁移状态

| 组件 | 状态 | 说明 |
|------|------|------|
| DataWriter<T>/DataReader<T> | Deprecated | 依赖 T::serialize/deserialize，已标记 @deprecated |
| DataWriterRaw/DataReaderRaw | 推荐使用 | 传输原始字节，上层负责序列化 |
| ProtoPublisher<T>/ProtoSubscriber<T> | 推荐使用 | 封装 Raw 接口 + protobuf 序列化 |

---

## 2. 旧 API vs 新 API

### 2.1 发布者对比

**旧 API (已废弃)**

```cpp
// 需要类型定义 serialize() 方法
struct MyData {
    std::string message;
    int value;

    // 旧API需要自定义序列化
    std::vector<uint8_t> serialize() const {
        // ... 手动序列化逻辑
    }
};

// 使用旧API
auto participant = moss::mdds::DomainParticipant::create(0);
auto publisher = participant->create_publisher<MyData>("my_topic");
publisher->write(my_data);
```

**新 API (推荐)**

```cpp
// 使用 protobuf 消息
#include "mcom/topic/proto_publisher.h"

auto node = moss::mcom::Node::create("my_node");
auto pub = node->create_publisher<moss::proto::topic::Image>("/camera/image");

moss::proto::topic::Image img;
img.set_width(1920);
img.set_height(1080);
pub->publish(img);
```

### 2.2 订阅者对比

**旧 API (已废弃)**

```cpp
// 需要类型定义 deserialize() 方法
struct MyData {
    std::string message;
    int value;

    // 旧API需要自定义反序列化
    static MyData deserialize(const uint8_t* data, size_t size) {
        // ... 手动反序列化逻辑
    }
};

// 使用旧API
auto participant = moss::mdds::DomainParticipant::create(0);
auto subscriber = participant->create_subscriber<MyData>("my_topic",
    [](const MyData& data, uint64_t ts) {
        fmt::print("收到: {}={}\n", data.message, data.value);
    });
```

**新 API (推荐)**

```cpp
// 使用 protobuf 消息
#include "mcom/topic/proto_subscriber.h"

auto node = moss::mcom::Node::create("my_node");
auto sub = node->create_subscriber<moss::proto::topic::Image>("/camera/image",
    [](const moss::proto::topic::Image& img, uint64_t ts) {
        fmt::print("收到图像: {}x{}\n", img.width(), img.height());
    });
sub->subscribe();
```

---

## 3. 迁移步骤

### 3.1 从 mdds::Publisher<T> 迁移到 ProtoPublisher<T>

**步骤 1：定义 Protobuf 消息**

```protobuf
// proto/topic/my_data.proto
syntax = "proto3";
package moss.proto.topic;

message MyData {
    string message = 1;
    int32 value = 2;
}
```

**步骤 2：使用 ProtoPublisher**

```cpp
#include "mcom/topic/proto_publisher.h"

auto node = moss::mcom::Node::create("my_node");
auto pub = node->create_publisher<moss::proto::topic::MyData>("my_topic");

moss::proto::topic::MyData data;
data.set_message("hello");
data.set_value(42);
pub->publish(data);
```

### 3.2 从 mdds::Subscriber<T> 迁移到 ProtoSubscriber<T>

**步骤 1：使用相同的 Protobuf 消息定义**

**步骤 2：使用 ProtoSubscriber**

```cpp
#include "mcom/topic/proto_subscriber.h"

auto node = moss::mcom::Node::create("my_node");
auto sub = node->create_subscriber<moss::proto::topic::MyData>("my_topic",
    [](const moss::proto::topic::MyData& data, uint64_t timestamp) {
        fmt::print("收到: {}={}\n", data.message(), data.value());
    });
sub->subscribe();
```

### 3.3 从自定义序列化迁移到 Protobuf

如果之前使用了自定义的 `serialize()`/`deserialize()` 方法：

**旧代码**

```cpp
struct SensorData {
    float temperature;
    float humidity;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buf;
        // ... 手动序列化
        return buf;
    }

    static SensorData deserialize(const uint8_t* data, size_t size) {
        SensorData obj;
        // ... 手动反序列化
        return obj;
    }
};
```

**新代码**

```protobuf
// proto/topic/sensor_data.proto
syntax = "proto3";
package moss.proto.topic;

message SensorData {
    float temperature = 1;
    float humidity = 2;
}
```

```cpp
// 使用方式
moss::proto::topic::SensorData data;
data.set_temperature(25.5f);
data.set_humidity(60.0f);
pub->publish(data);
```

---

## 4. Node API 统一入口

mcom 提供 `Node` 类作为统一入口：

```cpp
auto node = moss::mcom::Node::create("my_node");

// 创建发布者
auto pub = node->create_publisher<TopicType>("/topic/name");

// 创建订阅者
auto sub = node->create_subscriber<TopicType>("/topic/name",
    [](const TopicType& msg, uint64_t ts) {
        // 处理消息
    });

// 创建服务客户端
auto client = node->create_service_client<ServiceType>("/service/name");

// 创建动作客户端
auto action = node->create_action_client<ActionType>("/action/name");

// 启动事件循环
node->spin();
```

---

## 5. 常见问题

### 5.1 为什么选择 Protobuf 而不是自定义序列化？

- **跨语言支持**：Protobuf 支持 C++、Python、Java 等多语言
- **版本兼容**：字段添加/删除不影响旧代码
- **性能**：高效的二进制序列化
- **工具支持**：完整的代码生成工具链

### 5.2 旧代码还能用吗？

`DataWriter<T>`/`DataReader<T>` 已标记为 `@deprecated`，但暂时不影响现有功能。建议尽快迁移到新 API。

### 5.3 如何处理迁移过渡期？

如果暂时无法迁移所有代码，可以：

1. 新代码使用 ProtoPublisher/ProtoSubscriber
2. 旧代码继续使用 DataWriter/DataReader（已标记 deprecated）
3. 两者可以共存，不互相影响

---

## 6. 参考文档

- [Mcom_Design.md](./Mcom_Design.md) - mcom 设计文档
- [Mcom_Protobuf_Design.md](./Mcom_Protobuf_Design.md) - Protobuf 集成设计
- [mdds 设计文档](../mdds/doc/) - mdds 模块设计
- [proto 消息定义](../proto/) - Protobuf 消息定义

---

**更新日志**

- 2026-04-02: v1.0 初始版本
