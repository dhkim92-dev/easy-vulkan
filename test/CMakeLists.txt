# cmake_minimum_required(VERSION 3.14)

# project(test VERSION 1.0 LANGUAGES CXX)

# # Find the easy-vulkan library
# # find_package(easy-vulkan REQUIRED)

# # GoogleTest 다운로드 및 빌드
# include(FetchContent)
# FetchContent_Declare(
#     googletest
#     URL https://github.com/google/googletest/archive/refs/heads/main.zip
# )
# # gtest 빌드 옵션 설정 (테스트만 빌드)
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# enable_testing()
# file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_codes/*.cpp")
# file(GLOB TEST_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/test_codes/*.h")
# file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*")


# add_executable(test_main ${TEST_SOURCES} ${TEST_HEADERS})

# target_compile_definitions(test_main PRIVATE
#     EASY_VULKAN_ROOT_DIR="${CMAKE_SOURCE_DIR}"
# )

# # Link the easy-vulkan library to the test executable
# target_link_libraries(test_main PRIVATE easy-vulkan gtest gtest_main)

# add_custom_target(copy_test_shaders ALL
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
#             "${CMAKE_CURRENT_BINARY_DIR}/shaders"
#     COMMENT "Copying test shaders to build directory"
# )

# # include 디렉토리 추가 (필요시)
# target_include_directories(test_main PRIVATE
#     ${CMAKE_SOURCE_DIR}/easy-vulkan/include
# )

# # 테스트 등록
cmake_minimum_required(VERSION 3.14)

project(easy-vulkan-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# target_compile_definitions(test_main PRIVATE
#     EASY_VULKAN_ROOT_DIR="${CMAKE_SOURCE_DIR}"
# )

# GoogleTest 다운로드 및 빌드
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# gtest 빌드 옵션 설정 (테스트만 빌드)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# CTest 프레임워크 활성화
enable_testing()

# 소스 파일 목록 (GLOB은 새 파일 추가 시 CMake 캐시를 정리해야 할 수 있습니다)
file(GLOB TEST_SOURCES "test_codes/*.cpp")

# 테스트 실행 파일 생성
add_executable(test_main ${TEST_SOURCES})
target_compile_definitions(test_main PRIVATE
    EASY_VULKAN_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# easy-vulkan 라이브러리의 헤더 경로 추가
target_include_directories(test_main PRIVATE
    "${CMAKE_SOURCE_DIR}/easy-vulkan/include"
)

# easy-vulkan 및 Google Test 라이브러리 링크
# GTest::gtest 타겟을 사용하면 헤더 경로가 자동으로 포함됩니다.
target_link_libraries(test_main PRIVATE
    easy-vulkan
    GTest::gtest
    GTest::gtest_main
)

# 셰이더 파일을 빌드 디렉토리로 복사하는 커스텀 타겟
add_custom_target(copy_test_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMENT "Copying test shaders to build directory"
)

# GoogleTest 모듈을 포함하고 테스트를 자동으로 발견
include(GoogleTest)
gtest_discover_tests(test_main)