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

# 4. stb_image.h를 소스 및 빌드 디렉터리로 복사하는 커스텀 타겟 생성
add_custom_command(
    OUTPUT ${STB_HEADER_DST_SRC_DIR} ${STB_HEADER_DST_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STB_HEADER_SRC} ${STB_HEADER_DST_SRC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STB_HEADER_SRC} ${STB_HEADER_DST_BUILD_DIR}
    DEPENDS ${STB_HEADER_SRC}
    COMMENT "Copying stb_image.h to source and build directories"
)

add_custom_target(copy_stb_header ALL
    DEPENDS ${STB_HEADER_DST_SRC_DIR} ${STB_HEADER_DST_BUILD_DIR}
)

# 5. 다른 타겟들이 stb_image.h를 쉽게 사용하도록 INTERFACE 라이브러리 타겟 생성
#    이 타겟을 링크하면 현재 소스 디렉터리가 include 경로에 추가됩니다.
add_library(stb_headers INTERFACE)
target_include_directories(stb_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(stb_headers copy_stb_header)

# 6. 중복 정의 오류를 방지하기 위해 stb 구현부를 포함할 별도의 OBJECT 라이브러리 생성
#    이 파일을 external/stb/stb_image_impl.cpp 로 생성해야 합니다.
add_library(stb_impl OBJECT stb_impl.cpp)
target_include_directories(stb_impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(stb_impl copy_stb_header)