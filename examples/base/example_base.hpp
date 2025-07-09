#include "easy-vulkan.h"
#include <vector>
#include <string>
#include <memory>
#include <filesystem>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace std;

#define RUN_EXAMPLE_MAIN(IMPLEMENTED_CLASS_NAME, EXAMPLE_NAME) \
    int main(int argc, char** argv) { \
        ExampleBase* example = new IMPLEMENTED_CLASS_NAME(std::string(EXAMPLE_NAME), std::string(argv[0]), true); \
        example->run(); \
        delete example; \
        return 0; \
    } 

class ExampleBase {

    public: 

    ExampleBase(std::string example_name, std::string executable_path, bool debug = false) 
    : title(example_name), executable_path(executable_path), debug(debug) {
        ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::INFO);
        printf("ExampleBase constructor called with title: %s\n", title.c_str());
        std::filesystem::path bin_path = std::filesystem::path(executable_path).parent_path();
        std::filesystem::path base_path = bin_path.parent_path().parent_path();

        shader_path = base_path.parent_path() / "shaders";

        resource_path = base_path.parent_path() / "resources";

        if (debug) {
            ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::DEBUG);
        }
    };

    virtual ~ExampleBase() {
        if (swapchain) {
            swapchain->destroy();
        }

        vkDestroySurfaceKHR(instance->get_instance(), surface, nullptr);

        if (device) {
            device->destroy();
        }
        if (instance) {
            instance->destroy();
        }

        swapchain.reset();
        device.reset();
        physical_device.reset();
        instance.reset();
    }

    void run() {
        while(!glfwWindowShouldClose(display.window)) {
            glfwPollEvents();
            render();
        }
        glfwDestroyWindow(display.window);
        glfwTerminate();
    }

    protected:

    std::string title = "Example Base";

    std::filesystem::path executable_path;

    std::filesystem::path shader_path;

    std::filesystem::path resource_path;

    std::shared_ptr<ev::Instance> instance;

    std::shared_ptr<ev::PhysicalDevice> physical_device;

    std::shared_ptr<ev::Device> device;

    std::shared_ptr<ev::Swapchain> swapchain;

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    bool debug = false;

    struct Display{
        GLFWwindow* window = nullptr;
        uint32_t width = 800;
        uint32_t height = 600;
        vector<shared_ptr<ev::Framebuffer>> framebuffers;
    } display;

    virtual void setup_default_context() {
        init_window();
        vector<const char*> required_extensions = setup_instance_extensions();
        vector<const char*> required_layers = setup_instance_layers();
        vector<const char*> device_extensions = setup_device_extensions();

        if (debug) {
            required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            required_layers.push_back("VK_LAYER_KHRONOS_validation");
            ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::DEBUG);
        }

        instance = make_shared<ev::Instance>(required_extensions, required_layers, debug);
        setup_surface();
        physical_device = make_shared<ev::PhysicalDevice>(instance, instance->get_physical_devices()[0]);
        device = make_shared<ev::Device>(instance, physical_device, device_extensions, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
        swapchain = make_shared<ev::Swapchain>(instance, physical_device, device);
        swapchain->create(surface, display.width, display.height, true, false); // 예시로 800x600 크기, VSync 활성화
    }

    virtual void init_window() {
        if (!glfwInit()) {
            ev::logger::Logger::getInstance().error("Failed to initialize GLFW");
            exit(EXIT_FAILURE);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan API 사용을 위해 OpenGL API 사용 안함
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // 창 크기 조절 가능
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE); // 창을 보이도록 설정
        display.window = glfwCreateWindow(display.width, display.height, title.c_str(), nullptr, nullptr);
    }

    virtual void setup_surface() {
        ev::logger::Logger::getInstance().debug("Creating Vulkan surface for GLFW window... surface handler current hex : " + std::to_string(reinterpret_cast<uint64_t>(surface)));
        CHECK_RESULT(glfwCreateWindowSurface(*instance, display.window, nullptr, &surface));
        ev::logger::Logger::getInstance().debug("surface create complete surface handler current hex : " + std::to_string(reinterpret_cast<uint64_t>(surface)));
    }

    virtual vector<const char*> setup_instance_extensions() {
        
        uint32_t extension_count = 0;
        const char** glfw_required_extensions = glfwGetRequiredInstanceExtensions(&extension_count);

        vector<const char *> default_extensions(glfw_required_extensions, glfw_required_extensions + extension_count);
        if (debug) {
            default_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return default_extensions;
    }

    virtual vector<const char*> setup_device_extensions() {
        return vector<const char*> { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME };
    }

    virtual vector<const char*> setup_instance_layers() {
        vector<const char*> layers;
        if (debug) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }
        return layers;
    }

    virtual void render() = 0;

};