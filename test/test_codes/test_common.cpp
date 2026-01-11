#include "test_common.h"
#include "easy-vulkan.h"

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

void create_default_test_context(
    std::shared_ptr<ev::Instance>& instance,
    std::shared_ptr<ev::PhysicalDevice>& physical_device,
    std::shared_ptr<ev::Device>& device,
    bool debug
) {

    vector<const char*> required_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        #ifdef __APPLE__
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
    };
    vector<const char*> required_layers;

    if ( debug == true ) {
        required_layers.push_back("VK_LAYER_KHRONOS_validation");
        required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    } else {
    }
    instance = std::make_shared<ev::Instance>(required_extensions, required_layers, debug);
    auto physical_devices = instance->get_physical_devices();
    physical_device = std::make_shared<ev::PhysicalDevice>(instance, physical_devices[0]);
    vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    device = std::make_shared<ev::Device>(instance, physical_device, device_extensions);
}