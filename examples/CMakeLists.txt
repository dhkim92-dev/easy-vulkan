cmake_minimum_required(VERSION 3.14)

project(easy-vulkan-examples VERSION 1.0 LANGUAGES CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SAMPLE_MAIN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/samples/*/main.cpp")

# Create the test executable
# set(glfw_DIR "/opt/homebrew/lib/cmake/glfw3")
include_directories(
    ${CMAKE_SOURCE_DIR}/easy-vulkan/include
    ${CMAKE_SOURCE_DIR}/external/stb
    ${CMAKE_SOURCE_DIR}/externals/tiny_gltf)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

foreach(SAMPLE_MAIN ${SAMPLE_MAIN_FILES})
    # 예제 이름 추출 (samples/triangle/main.cpp -> triangle)
    get_filename_component(SAMPLE_DIR ${SAMPLE_MAIN} DIRECTORY)
    get_filename_component(EXAMPLE_NAME ${SAMPLE_DIR} NAME)

    # 실행 파일 경로 지정
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/examples/bin/${EXAMPLE_NAME})
    message(STATUS "Setting output directory for ${EXAMPLE_NAME} to ${EXECUTABLE_OUTPUT_PATH}")

    # 실행 파일 생성
    add_executable(${EXAMPLE_NAME}_example ${SAMPLE_MAIN})

        # easy-vulkan: on Windows use the static target; otherwise use the shared alias
        if(WIN32)
            set(EASY_VULKAN_TARGET easy-vulkan_static)
        else()
            set(EASY_VULKAN_TARGET easy-vulkan)
        endif()

        target_link_libraries(${EXAMPLE_NAME}_example PRIVATE ${EASY_VULKAN_TARGET} glfw Vulkan::Vulkan stb_headers stb_impl tiny_gltf_headers tiny_gltf_impl glm::glm)

    if(WIN32) 
    else()
    endif()


    # include 디렉터리 추가
    target_include_directories(${EXAMPLE_NAME}_example PRIVATE
        ${CMAKE_SOURCE_DIR}/easy-vulkan/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # 실행 파일 출력 디렉터리 지정
    set_target_properties(${EXAMPLE_NAME}_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
    )
endforeach()

# add_custom_command(
    # TARGET ${EXAMPLE_NAME}_example POST_BUILD
    # COMMAND ${CMAKE_COMMAND} -E copy_directory
        # ${CMAKE_SOURCE_DIR}/shaders
        # ${CMAKE_BINARY_DIR}/examples/shaders
    # COMMENT "Copying shaders directory to build/examples/shaders"
# )
