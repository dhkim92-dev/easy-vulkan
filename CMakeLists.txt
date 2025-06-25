cmake_minimum_required(VERSION 3.14)

project(easy-vulkan-project VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subdirectories for the easy-vulkan library and the test projec

option(BUILD_TESTS "Build tests" OFF) # 기본값 OFF
add_subdirectory(easy-vulkan)
add_subdirectory(local)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "빌드 디렉토리는 소스 디렉토리와 분리해야 합니다. 예: cmake -S . -B build")
endif()