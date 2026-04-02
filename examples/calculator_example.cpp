#include "mcom/service/mservice.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

using namespace moss::mcom::service;

static std::vector<uint8_t> int32_to_bytes(int32_t value) {
    std::vector<uint8_t> result(4);
    result[0] = (value >> 0) & 0xFF;
    result[1] = (value >> 8) & 0xFF;
    result[2] = (value >> 16) & 0xFF;
    result[3] = (value >> 24) & 0xFF;
    return result;
}

static int32_t bytes_to_int32(const std::vector<uint8_t>& data, size_t offset) {
    int32_t result = 0;
    result |= static_cast<int32_t>(data[offset]) << 0;
    result |= static_cast<int32_t>(data[offset + 1]) << 8;
    result |= static_cast<int32_t>(data[offset + 2]) << 16;
    result |= static_cast<int32_t>(data[offset + 3]) << 24;
    return result;
}

class CalculatorServer {
public:
    CalculatorServer()
        : server_(std::make_shared<ServiceServer>(0x1234, 0x0001)) {
    }

    void init() {
    }

private:
    ServiceServerPtr server_;
};

int main() {
    std::cout << "Calculator example - use mservice as library for service calls" << std::endl;
    std::cout << "This example demonstrates the API usage pattern" << std::endl;
    return 0;
}
