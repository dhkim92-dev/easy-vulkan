#include "easy-vulkan.h"
#include <vector>
#include <string>
#include <memory>
#include <filesystem>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace std;

#define RUN_EXAMPLE_MAIN(IMPLEMENTED_CLASS_NAME, EXAMPLE_NAME, DEBUG) \
    int main(int argc, char** argv) { \
        ExampleBase* example = new IMPLEMENTED_CLASS_NAME(std::string(EXAMPLE_NAME), std::string(argv[0]), DEBUG); \
        example->run(); \
        delete example; \
        return 0; \
    } 

class ExampleBase {

    public: 

    ExampleBase(std::string example_name, std::string executable_path, bool debug = false) 
    : title(example_name), executable_path(executable_path), debug(debug) {
        ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::INFO);
        std::filesystem::path bin_path = std::filesystem::canonical(executable_path);
        std::filesystem::path base_path = bin_path.parent_path().parent_path().parent_path();

        shader_path = base_path.parent_path() / "shaders";
        auto build_path = base_path.parent_path();
        resource_path = build_path.parent_path() /  "resources";

        ev::logger::Logger::getInstance().info("Executable path set to: " + executable_path);
        ev::logger::Logger::getInstance().info("Shader path set to: " + shader_path.string());
        ev::logger::Logger::getInstance().info("Resource path set to: " + resource_path.string());

        if (debug) {
            ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::DEBUG);
        }

        setup_default_context();
    };

    virtual ~ExampleBase() {
        pre_destroy();

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

    struct DepthStencil {
        std::shared_ptr<ev::Image> image;
        std::shared_ptr<ev::Memory> memory;
        std::shared_ptr<ev::ImageView> view;
    };

    std::shared_ptr<ev::Swapchain> swapchain;

    std::vector<std::shared_ptr<ev::Framebuffer>> framebuffers;

    std::vector<DepthStencil> depth_stencils;

    std::shared_ptr<ev::RenderPass> render_pass;

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    bool debug = false;

    struct Display{
        GLFWwindow* window = nullptr;
        uint32_t width = 800;
        uint32_t height = 600;
        vector<shared_ptr<ev::Framebuffer>> framebuffers;
    } display;

    bool prepared = false;

    bool resized = false;

    uint32_t target_width;

    uint32_t target_height;

    std::shared_ptr<ev::Semaphore> render_complete;

    std::shared_ptr<ev::Semaphore> present_complete;

    virtual void setup_default_context() {
        init_window();
        std::vector<const char*> required_extensions = setup_instance_extensions();
        std::vector<const char*> required_layers = setup_instance_layers();
        std::vector<const char*> device_extensions = setup_device_extensions();

        if (debug) {
            required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            required_layers.push_back("VK_LAYER_KHRONOS_validation");
            ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::DEBUG);
        }

        instance = std::make_shared<ev::Instance>(required_extensions, required_layers, debug);
        setup_surface();
        physical_device = std::make_shared<ev::PhysicalDevice>(instance, instance->get_physical_devices()[0]);
        device = std::make_shared<ev::Device>(instance, physical_device, device_extensions, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

        create_swapchain();
        create_depth_stencil_buffers();
        create_framebuffers();
        setup_sync();
    }

    virtual void init_window() {
        if (!glfwInit()) {
            ev::logger::Logger::getInstance().error("Failed to initialize GLFW");
            exit(EXIT_FAILURE);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); 
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        display.window = glfwCreateWindow(display.width, display.height, title.c_str(), nullptr, nullptr);

        if (!display.window) {
            ev::logger::Logger::getInstance().error("Failed to create GLFW window");
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
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


    virtual void on_window_resize() {
        if ( !prepared ) {
            return;
        }

        prepared = false;
        resized = true;
        device->wait_idle(UINT32_MAX);
        display.width = target_width;
        display.height = target_height;
        destroy_graphics_pipeline();
        destroy_framebuffers();
        destroy_swapchain();
        create_swapchain();
        create_depth_stencil_buffers();
        create_framebuffers();
        create_graphics_pipelines();
        prepared = true;
    }

    virtual void pre_destroy() {};
    
    virtual void create_depth_stencil_buffers() {
        depth_stencils.clear();

        for ( uint32_t i = 0 ;i < swapchain->get_images().size() ; ++i ) {
            DepthStencil ds;
            ds.image = std::make_shared<ev::Image>(
                device,
                VK_IMAGE_TYPE_2D,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                display.width,
                display.height,
                1, 1, 1,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TILING_OPTIMAL
            );

            ds.memory = std::make_shared<ev::Memory>(
                device,
                ds.image->get_memory_requirements().size,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                ds.image->get_memory_requirements()
            );

            ds.image->bind_memory(ds.memory);

            VkComponentMapping cm = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            };

            VkImageSubresourceRange sr = {
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                0, 1, 0, 1
            };

            ds.view = std::make_shared<ev::ImageView>(
                device,
                ds.image,
                VK_IMAGE_VIEW_TYPE_2D,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                cm,
                sr
            );

            depth_stencils.push_back(ds);
        } 
    }

    virtual void create_swapchain() {
        swapchain = std::make_shared<ev::Swapchain>(instance, physical_device, device);
        swapchain->create(surface, display.width, display.height, true, false); // 예시로 800x600 크기, VSync 활성화
    }

    virtual void create_framebuffers() {
        // Basically use color and Depth stencil framebuffer creation.
        for (uint32_t i = 0 ; i < swapchain->get_image_views().size() ; ++i ) {
            VkImageView view = swapchain->get_image_views()[i];
            DepthStencil& depth_stencil = depth_stencils[i];
            vector<VkImageView> attachments = {view, *depth_stencil.view};
            std::shared_ptr<ev::Framebuffer> fb = std::make_shared<ev::Framebuffer>(
                device,
                render_pass,
                attachments,
                display.width,
                display.height,
                1
            );
            framebuffers.push_back(fb);
        } 
    }

    virtual void destroy_framebuffers() {
        framebuffers.clear();
        depth_stencils.clear();
    }

    virtual void create_graphics_pipelines() = 0;

    virtual void destroy_swapchain() {
        swapchain.reset();
    }

    virtual void setup_sync() {
        render_complete = std::make_shared<ev::Semaphore>(device);
        present_complete = std::make_shared<ev::Semaphore>(device);
        prepared = true;
    }

    virtual void destroy_graphics_pipeline() = 0;

    virtual void render() = 0;
};
