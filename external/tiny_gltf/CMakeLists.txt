cmake_minimum_required(VERSION 3.14)
project(tinygltf-dependency NONE)

include(FetchContent)

FetchContent_Declare(
    tiny_gltf_fetch
    GIT_REPOSITORY https://github.com/syoyo/tinygltf
    GIT_TAG "v2.9.6"
)

FetchContent_GetProperties(tiny_gltf_fetch)
if(NOT tiny_gltf_fetch_POPULATED)
    FetchContent_Populate(tiny_gltf_fetch)
endif()

set(TINYGLTF_HEADER_SRC ${tiny_gltf_fetch_SOURCE_DIR}/tiny_gltf.h)
set(TINYGLTF_HEADER_DST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tiny_gltf.h)
set(TINYGLTF_HEADER_DST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/tiny_gltf.h)
set(TINYGLTF_HEADER_IMPL_SRC ${tiny_gltf_fetch_SOURCE_DIR}/json.hpp)
set(TINYGLTF_HEADER_IMPL_DST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/json.hpp)
set(TINYGLTF_HEADER_IMPL_DST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/json.hpp)

add_custom_command(
    OUTPUT ${TINYGLTF_HEADER_DST_SRC_DIR} ${TINYGLTF_HEADER_DST_BUILD_DIR}
           ${TINYGLTF_HEADER_IMPL_DST_SRC_DIR} ${TINYGLTF_HEADER_IMPL_DST_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TINYGLTF_HEADER_SRC} ${TINYGLTF_HEADER_DST_SRC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TINYGLTF_HEADER_SRC} ${TINYGLTF_HEADER_DST_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TINYGLTF_HEADER_IMPL_SRC} ${TINYGLTF_HEADER_IMPL_DST_SRC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TINYGLTF_HEADER_IMPL_SRC} ${TINYGLTF_HEADER_IMPL_DST_BUILD_DIR}
    DEPENDS ${TINYGLTF_HEADER_SRC} ${TINYGLTF_HEADER_IMPL_SRC}
    COMMENT "Copying tiny_gltf.h and json.hpp to source and build directories"
)

add_custom_target(copy_tinygltf_header ALL
    DEPENDS ${TINYGLTF_HEADER_DST_SRC_DIR} ${TINYGLTF_HEADER_DST_BUILD_DIR}
            ${TINYGLTF_HEADER_IMPL_DST_SRC_DIR} ${TINYGLTF_HEADER_IMPL_DST_BUILD_DIR}
)

add_library(tiny_gltf_headers INTERFACE)
target_include_directories(tiny_gltf_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(tiny_gltf_headers copy_tinygltf_header)
target_link_libraries(tiny_gltf_headers INTERFACE stb_headers)

add_library(tiny_gltf_impl OBJECT tinygltf_impl.cpp)
target_include_directories(tiny_gltf_impl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(tiny_gltf_impl copy_tinygltf_header)
target_link_libraries(tiny_gltf_impl PUBLIC stb_headers)