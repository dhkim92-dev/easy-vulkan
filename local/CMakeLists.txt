cmake_minimum_required(VERSION 3.14)

project(test VERSION 1.0 LANGUAGES CXX)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB TEST_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# Create the test executable
set(glfw_DIR "/opt/homebrew/lib/cmake/glfw3")
include_directories(${CMAKE_SOURCE_DIR}/easy-vulkan/include)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
add_executable(local_main ${TEST_SOURCES} ${TEST_HEADERS})

target_compile_definitions(local_main PRIVATE
    EASY_VULKAN_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Link the easy-vulkan library to the test executable
target_link_libraries(local_main PRIVATE easy-vulkan glfw vulkan)

# include 디렉토리 추가 (필요시)
target_include_directories(local_main PRIVATE
    ${CMAKE_SOURCE_DIR}/easy-vulkan/include
)