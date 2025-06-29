cmake_minimum_required(VERSION 3.14)

project(easy-vulkan-shaders NONE)

# glslc가 시스템 PATH에 있다고 가정
find_program(GLSLC_EXECUTABLE glslc REQUIRED)

# 모든 예제별 쉐이더 파일(.vert, .frag, .comp) 찾기
file(GLOB_RECURSE SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.comp"
)

set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")

set(SPV_FILES "")
foreach(SHADER ${SHADER_FILES})
    # 예제 디렉터리 이름 추출 (shaders/example_name/shader.vert)
    get_filename_component(EXAMPLE_DIR ${SHADER} DIRECTORY)
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_DIR} NAME)
    get_filename_component(SHADER_FILE ${SHADER} NAME)
    set(SPV_OUTPUT_DIR "${SHADER_OUTPUT_DIR}/${EXAMPLE_NAME}")
    file(MAKE_DIRECTORY ${SPV_OUTPUT_DIR})
    set(SPV_FILE "${SPV_OUTPUT_DIR}/${SHADER_FILE}.spv")

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SPV_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER} to ${SPV_FILE}"
    )
    list(APPEND SPV_FILES ${SPV_FILE})
endforeach()

add_custom_target(compile_all_shaders ALL DEPENDS ${SPV_FILES})