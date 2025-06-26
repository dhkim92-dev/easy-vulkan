#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "easy-vulkan.h"

// using namespace ev;
using namespace std;

int main(void) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // glfw vulkan 이용을 위한 extension listing
    if (!glfwVulkanSupported()) {
        std::cerr << "Vulkan is not supported by GLFW" << std::endl;
        return -1;
    }
    // 필요한 확장 기능을 가져오기
    uint32_t extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    if (!glfw_extensions) {
        std::cerr << "Failed to get required Vulkan extensions from GLFW" << std::endl;
        return -1;
    }

    vector<const char*> required_extensions(glfw_extensions, glfw_extensions + extension_count);
    // 필요한 확장 기능을 출력
    std::cout << "Required Vulkan extensions:" << std::endl;
    for (const auto& ext : required_extensions) {
        std::cout << " - " << ext << std::endl;
    }

    if (required_extensions.empty()) {
        std::cerr << "No required Vulkan extensions found" << std::endl;
        return -1;
    }   


    // 디버그 레이어 활성화
    vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation" // 디버그 레이어 활성화
    };

    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // 디버그 유틸리티 확장 추가

    // Vulkan Surface 생성
    shared_ptr<ev::Instance> instance = make_shared<ev::Instance>(
        required_extensions, // 필요한 확장 기능
        validation_layers, // 필요한 레이어
        true
    );

    if (!instance->is_valid()) {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
        return -1;
    }

    vector<VkPhysicalDevice> physical_devices = ev::utility::list_physical_devices(instance->get_instance());

    if (physical_devices.empty()) {
        std::cerr << "No Vulkan physical devices found" << std::endl;
        return -1;
    }



    shared_ptr<ev::PhysicalDevice> physical_device = make_shared<ev::PhysicalDevice>(
        instance, // Vulkan 인스턴스
        physical_devices[0] // 첫 번째 물리 장치 선 택
    );

    shared_ptr<ev::Device> device = make_shared<ev::Device>(
        instance, // Vulkan 인스턴스
        physical_device, // 물리 장치
        vector<const char*>{"VK_KHR_swapchain"}, 
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT // 그래픽, 컴퓨트, 트랜스퍼 큐 플래그
    );
    shared_ptr<ev::Swapchain> swapchain = make_shared<ev::Swapchain>(
        instance, // Vulkan 인스턴스
        physical_device, // 물리 장치
        device // 논리 장치
    );
    uint32_t width = 800, height = 600; // 초기 윈도우 크기

    // vulkan용 glfw 윈도우 생성
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan API를 사용하기 위해 OpenGL을 사용하지 않도록 설정
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Window", nullptr, nullptr);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance->get_instance(), window, nullptr, &surface) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan surface" << std::endl;
        return -1;
    }
    swapchain->create(surface, width, height, true, false);


    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    shared_ptr<ev::CommandPool> command_pool = make_shared<ev::CommandPool>(device, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

    vector<shared_ptr<ev::CommandBuffer>> command_buffers = command_pool->allocate(3, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    // shared_ptr<ev::CommandBuffer> command_buffer = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    while( !glfwWindowShouldClose(window)) {
        glfwPollEvents(); // 이벤트 처ㅏㅏㅏ
    }

    glfwDestroyWindow(window);
    glfwTerminate(); // GLFW 종료

    return 0;
}