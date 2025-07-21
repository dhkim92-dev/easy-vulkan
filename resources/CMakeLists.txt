# Python 3 인터프리터를 찾습니다.
cmake_minimum_required(VERSION 3.14)

project(easy-vulkan-resources NONE)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# 스크립트와 JSON 파일의 경로를 지정합니다.
set(DOWNLOAD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/download_assets.py)
set(RESOURCES_JSON ${CMAKE_CURRENT_SOURCE_DIR}/resources.json)

# 스크립트 실행을 추적하기 위한 스탬프 파일을 빌드 디렉터리에 지정합니다.
set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/download_stamp.txt)

# Python 다운로드 스크립트를 실행하는 사용자 정의 명령입니다.
# 이 명령은 스탬프 파일이 없거나, 스크립트 또는 JSON 파일이 변경되었을 때 실행됩니다.
add_custom_command(
    OUTPUT ${STAMP_FILE}
    COMMAND ${Python3_EXECUTABLE} ${DOWNLOAD_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${DOWNLOAD_SCRIPT} ${RESOURCES_JSON}
    COMMENT "Running asset download script (download_assets.py)..."
    VERBATIM
)

# 다운로드 명령을 실행하는 사용자 정의 타겟입니다.
# 'ALL' 옵션으로 기본 빌드에 포함됩니다.
add_custom_target(download_resources ALL
    DEPENDS ${STAMP_FILE}
)

# 더 이상 사용되지않음 
# 'download_resources' 타겟이 빌드된 후, 리소스 디렉터리의 모든 내용을
# 빌드 트리 내의 해당 디렉터리로 복사합니다.
# add_custom_command(
#     TARGET download_resources POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             "${CMAKE_CURRENT_SOURCE_DIR}"
#             "${CMAKE_CURRENT_BINARY_DIR}"
#     COMMENT "Copying resources to build directory..."
#     VERBATIM
# )

# add_custom_command(
#     TARGET download_resources POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             "${CMAKE_CURRENT_SOURCE_DIR}/glTF-Sample-Models"
#             "${CMAKE_BINARY_DIR}/resources/glTF_Sample-Models"
#     COMMENT "Copying glTF-Sample-Models to build/resources/glTF_Sample-Models..."
#     VERBATIM
# )

