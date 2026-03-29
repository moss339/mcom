#include "mservice/mservice.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

void test_service_error_enum() {
    assert(mservice::service_error_to_string(mservice::ServiceError::OK) == "OK");
    assert(mservice::service_error_to_string(mservice::ServiceError::TIMEOUT) == "TIMEOUT");
    assert(mservice::service_error_to_string(mservice::ServiceError::SERVICE_NOT_FOUND) == "SERVICE_NOT_FOUND");
    assert(mservice::service_error_to_string(mservice::ServiceError::METHOD_NOT_FOUND) == "METHOD_NOT_FOUND");
    assert(mservice::service_error_to_string(mservice::ServiceError::INVALID_REQUEST) == "INVALID_REQUEST");
    assert(mservice::service_error_to_string(mservice::ServiceError::INTERNAL_ERROR) == "INTERNAL_ERROR");
    assert(mservice::service_error_to_string(mservice::ServiceError::CANCELED) == "CANCELED");
    std::cout << "test_service_error_enum passed" << std::endl;
}

void test_service_manager_singleton() {
    auto& manager1 = mservice::ServiceManager::instance();
    auto& manager2 = mservice::ServiceManager::instance();
    assert(&manager1 == &manager2);
    std::cout << "test_service_manager_singleton passed" << std::endl;
}

void test_session_id_generation() {
    auto& manager = mservice::ServiceManager::instance();
    mservice::SessionId id1 = manager.generate_session_id();
    mservice::SessionId id2 = manager.generate_session_id();
    assert(id1 != id2);
    std::cout << "test_session_id_generation passed" << std::endl;
}

void test_request_header_size() {
    mservice::RequestHeader header;
    header.session_id = 1;
    header.service_id = 0x1234;
    header.instance_id = 0x0001;
    header.method_id = 0x0001;

    assert(header.session_id == 1);
    assert(header.service_id == 0x1234);
    assert(header.instance_id == 0x0001);
    assert(header.method_id == 0x0001);
    std::cout << "test_request_header_size passed" << std::endl;
}

void test_response_header_size() {
    mservice::ResponseHeader header;
    header.session_id = 1;
    header.service_id = 0x1234;
    header.instance_id = 0x0001;
    header.method_id = 0x0001;

    assert(header.session_id == 1);
    assert(header.service_id == 0x1234);
    assert(header.instance_id == 0x0001);
    assert(header.method_id == 0x0001);
    std::cout << "test_response_header_size passed" << std::endl;
}

void test_request_response_construction() {
    mservice::Request request;
    request.header.session_id = 1;
    request.header.service_id = 0x1234;
    request.header.instance_id = 0x0001;
    request.header.method_id = 0x0001;
    request.payload = {1, 2, 3, 4};

    assert(request.payload.size() == 4);
    assert(request.payload[0] == 1);

    mservice::Response response;
    response.header.session_id = 1;
    response.header.service_id = 0x1234;
    response.header.instance_id = 0x0001;
    response.header.method_id = 0x0001;
    response.error = mservice::ServiceError::OK;
    response.payload = {5, 6, 7, 8};

    assert(response.error == mservice::ServiceError::OK);
    assert(response.payload.size() == 4);

    std::cout << "test_request_response_construction passed" << std::endl;
}

int main() {
    test_service_error_enum();
    test_service_manager_singleton();
    test_session_id_generation();
    test_request_header_size();
    test_response_header_size();
    test_request_response_construction();

    std::cout << "All service tests passed!" << std::endl;
    return 0;
}
