cmake_minimum_required(VERSION 3.14)

project(test VERSION 1.0 LANGUAGES CXX)

# Find the easy-vulkan library
# find_package(easy-vulkan REQUIRED)

# GoogleTest 다운로드 및 빌드
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# gtest 빌드 옵션 설정 (테스트만 빌드)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_codes/*.cpp")
file(GLOB TEST_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/test_codes/*.h")
file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*")


add_executable(test_main ${TEST_SOURCES} ${TEST_HEADERS})

target_compile_definitions(test_main PRIVATE
    EASY_VULKAN_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Link the easy-vulkan library to the test executable
target_link_libraries(test_main PRIVATE easy-vulkan gtest gtest_main)

add_custom_target(copy_test_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMENT "Copying test shaders to build directory"
)

# include 디렉토리 추가 (필요시)
target_include_directories(test_main PRIVATE
    ${CMAKE_SOURCE_DIR}/easy-vulkan/include
)

# 테스트 등록
add_test(NAME EasyVulkanTest COMMAND test_main)