cmake_minimum_required(VERSION 3.14)

project(easy-vulkan VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 헤더/소스 파일 자동 수집
file(GLOB EASY_VULKAN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB EASY_VULKAN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 동적 라이브러리 생성, 이름 형식 지정
set(LIBRARY_OUTPUT_NAME "easy-vulkan")
add_library(${LIBRARY_OUTPUT_NAME} SHARED ${EASY_VULKAN_SRC})

# 버전 및 출력 파일명 지정 (lib-easy-vulkan.1.0.so 등)
set_target_properties(${LIBRARY_OUTPUT_NAME} PROPERTIES
    OUTPUT_NAME "easy-vulkan"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PREFIX "lib-"
)


# Vulkan SDK 찾기 (OS별 지원)
find_package(Vulkan REQUIRED)
target_link_libraries(${LIBRARY_OUTPUT_NAME} PUBLIC Vulkan::Vulkan tiny_gltf_headers tiny_gltf_impl stb_headers stb_impl)

# include 디렉토리 공개
target_include_directories(${LIBRARY_OUTPUT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${Vulkan_INCLUDE_DIRS} 
)

# 설치 규칙: 라이브러리와 헤더를 함께 배포
install(TARGETS ${LIBRARY_OUTPUT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)
