#include "test_common.h"

// 이 소스 파일 내에서만 접근 가능한 static 전역 변수
static std::filesystem::path g_executable_dir;

// main 함수에서 호출되어 전역 변수를 초기화하는 함수의 '정의'
void init_executable_path(const char* executable_path_str) {
    g_executable_dir = std::filesystem::path(executable_path_str).parent_path();
}

// 다른 테스트 파일들이 경로를 가져갈 수 있도록 하는 함수의 '정의'
const std::filesystem::path& get_executable_dir() {
    return g_executable_dir;
}