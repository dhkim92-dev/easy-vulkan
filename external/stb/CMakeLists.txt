cmake_minimum_required(VERSION 3.14)
project(stb-dependency NONE)

include(FetchContent)

# 1. stb 라이브러리 의존성 선언
FetchContent_Declare(
    stb_fetch
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)

# 2. stb 라이브러리 소스 다운로드
FetchContent_GetProperties(stb_fetch)
if(NOT stb_fetch_POPULATED)
    FetchContent_Populate(stb_fetch)
endif()

# 3. 헤더 파일을 복사할 경로 설정
set(STB_HEADER_SRC ${stb_fetch_SOURCE_DIR}/stb_image.h)
set(STB_HEADER_DST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb_image.h)
set(STB_HEADER_DST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/stb_image.h)
set(STB_IMAGE_WRITE_HEADER_SRC ${stb_fetch_SOURCE_DIR}/stb_image_write.h)
set(STB_IMAGE_WRITE_DST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb_image_write.h)
set(STB_IMAGE_WRITE_DST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/stb_image_write.h)

# 4. stb_image.h, stb_image_write.h를 소스 및 빌드 디렉터리로 복사하는 커스텀 타겟 생성
add_custom_command(
    OUTPUT ${STB_HEADER_DST_SRC_DIR} ${STB_HEADER_DST_BUILD_DIR}
           ${STB_IMAGE_WRITE_DST_SRC_DIR} ${STB_IMAGE_WRITE_DST_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STB_HEADER_SRC} ${STB_HEADER_DST_SRC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STB_HEADER_SRC} ${STB_HEADER_DST_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STB_IMAGE_WRITE_HEADER_SRC} ${STB_IMAGE_WRITE_DST_SRC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STB_IMAGE_WRITE_HEADER_SRC} ${STB_IMAGE_WRITE_DST_BUILD_DIR}
    DEPENDS ${STB_HEADER_SRC}
    DEPENDS ${STB_IMAGE_WRITE_HEADER_SRC}
    COMMENT "Copying stb_image.h stb_image_write.h to source and build directories"
)

add_custom_target(copy_stb_header ALL
    DEPENDS ${STB_HEADER_DST_SRC_DIR} ${STB_HEADER_DST_BUILD_DIR}
            ${STB_IMAGE_WRITE_DST_SRC_DIR} ${STB_IMAGE_WRITE_DST_BUILD_DIR}
)

# 5. INTERFACE 라이브러리로 include 경로 제공
add_library(stb_headers INTERFACE)
target_include_directories(stb_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(stb_headers copy_stb_header)

# 6. OBJECT 라이브러리 stb_impl 생성 (구현부가 필요하다면)
add_library(stb_impl OBJECT stb_impl.cpp)
set_target_properties(stb_impl PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(stb_impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(stb_impl copy_stb_header)
