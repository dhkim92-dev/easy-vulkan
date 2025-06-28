#pragma once

#include <filesystem>
#include <memory>
#include "easy-vulkan.h"

// 실행 파일이 위치한 디렉터리 경로를 반환하는 함수의 '선언'
const std::filesystem::path& get_executable_dir();

// main 함수에서 경로를 초기화하기 위한 함수의 '선언'
void init_executable_path(const char* executable_path_str);

void create_default_test_context(
    std::shared_ptr<ev::Instance>& instance,
    std::shared_ptr<ev::PhysicalDevice>& physical_device,
    std::shared_ptr<ev::Device>& device,
    bool debug
);