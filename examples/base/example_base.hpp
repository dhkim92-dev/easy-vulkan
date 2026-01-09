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

    std::shared_ptr<ev::Queue> queue;

    struct DepthStencil {
        std::shared_ptr<ev::Image> image;
        std::shared_ptr<ev::Memory> memory;
        std::shared_ptr<ev::ImageView> view;
    };

    std::shared_ptr<ev::Swapchain> swapchain;

    std::vector<std::shared_ptr<ev::Framebuffer>> framebuffers;

    std::vector<DepthStencil> depth_stencils;

    std::shared_ptr<ev::RenderPass> render_pass;

    std::vector<std::shared_ptr<ev::Fence>> fences;

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

    std::vector<std::shared_ptr<ev::CommandBuffer>> command_buffers;

    std::vector<std::shared_ptr<ev::Semaphore>> render_completes;

    std::vector<std::shared_ptr<ev::Semaphore>> present_completes;

    uint32_t current_frame_index = 0;

    uint32_t max_frames_in_flight = 2;

    virtual void setup_default_context() {
        init_window();
        create_instance();
        create_surface();
        create_physical_device();
        create_logical_device();
        create_swapchain();
        create_queue();
        create_depth_stencil_buffers();
        create_renderpass();
        create_framebuffers();
        setup_sync();
    }

    virtual void create_instance() {
        std::vector<const char*> required_extensions = setup_instance_extensions();
        std::vector<const char*> required_layers = setup_instance_layers();
        
        if (debug) {
            required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            required_layers.push_back("VK_LAYER_KHRONOS_validation");
            ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::DEBUG);
        }

        instance = std::make_shared<ev::Instance>(required_extensions, required_layers, debug);
    }

    virtual void create_physical_device() {
        physical_device = std::make_shared<ev::PhysicalDevice>(instance, instance->get_physical_devices()[0]);
    }

    virtual void create_logical_device() {
        std::vector<const char*> device_extensions = setup_device_extensions();
        device = std::make_shared<ev::Device>(instance, physical_device, device_extensions, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
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

        glfwSetWindowUserPointer(display.window, this);
        glfwSetFramebufferSizeCallback(display.window, [](GLFWwindow* window, int width, int height) {
            std::printf("Framebuffer resize detected: width=%d, height=%d\n", width, height);
            ExampleBase* app = reinterpret_cast<ExampleBase*>(glfwGetWindowUserPointer(window));
            app->target_width = static_cast<uint32_t>(width);
            app->target_height = static_cast<uint32_t>(height);
            app->on_window_resize();
        });
    }

    virtual void create_surface() {
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

    virtual void create_queue() {
        ev::logger::Logger::getInstance().debug("[Setup Queue Start]");
        queue = std::make_shared<ev::Queue>(device, device->get_queue_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT));
        ev::logger::Logger::getInstance().debug("[Setup Queue End]");
    }

    virtual void on_window_resize() {
        if ( !prepared ) {
            return;
        }

        prepared = false;
        resized = true;

        device->wait_idle(UINT64_MAX); // 중요, 윈도우 리사이즈 콜백 수행 중에 GPU가 작업 중이면 안되므로 대기 
        
        display.width = target_width;
        display.height = target_height;

        destory_depth_stencil_buffers();
        destroy_framebuffers();
        destroy_swapchain();
        create_swapchain();
        create_depth_stencil_buffers();
        create_framebuffers();

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

    virtual void destory_depth_stencil_buffers() {
        depth_stencils.clear();
    }

    virtual void create_swapchain() {
        swapchain = std::make_shared<ev::Swapchain>(instance, physical_device, device);
        swapchain->create(surface, display.width, display.height, true, false); 
        max_frames_in_flight = static_cast<uint32_t>(swapchain->get_images().size());
    }

    virtual void destroy_swapchain() {
        swapchain.reset();
        max_frames_in_flight = 2;
    }

    virtual void create_framebuffers() {
        // Basically use color and Depth stencil framebuffer creation.
        std::printf("renderpass address : %lu\n", reinterpret_cast<uintptr_t>(VkRenderPass(*render_pass)));
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
    }

    virtual void setup_sync() {
        render_completes.resize(max_frames_in_flight);
        present_completes.resize(max_frames_in_flight);
        for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
            render_completes[i] = std::make_shared<ev::Semaphore>(device);
            present_completes[i] = std::make_shared<ev::Semaphore>(device);
        }
        prepared = true;
    }

    virtual void prepare_frame(bool wait_fences = false) {
        if ( wait_fences ) {
            std::vector<VkFence> vk_fences = {};
            for ( const auto& fence : fences ) {
                vk_fences.push_back(*fence);
            }
            CHECK_RESULT(vkWaitForFences(*device, static_cast<uint32_t>(vk_fences.size()), vk_fences.data(), VK_TRUE, UINT64_MAX));
            for ( const auto& fence : fences ) {
                fence->reset();
            }
        }

        VkResult result = swapchain->acquire_next_image(
            current_frame_index,
            present_completes[current_frame_index],
            VK_NULL_HANDLE
        );

        if ( (result == VK_ERROR_OUT_OF_DATE_KHR ) ||
             (result == VK_SUBOPTIMAL_KHR) ) {
                if( result == VK_ERROR_OUT_OF_DATE_KHR ) {
                    on_window_resize();
                }
            return;
        }

        CHECK_RESULT(result);
    }

    virtual void submit_frame() {
        VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        std::vector<std::shared_ptr<ev::Semaphore>> wait_semaphores = { present_completes[current_frame_index] };
        std::vector<std::shared_ptr<ev::Semaphore>> signal_semaphores = { render_completes[current_frame_index]  };

        VkResult result = queue->submit(
            command_buffers[current_frame_index],
            wait_semaphores,
            signal_semaphores,
            &wait_stage_mask,
            nullptr,
            nullptr
        );
        CHECK_RESULT(result);

        result = queue->present(
            swapchain,
            current_frame_index,
            { render_completes[current_frame_index] }
        );

        if ( (result == VK_ERROR_OUT_OF_DATE_KHR ) ||
             (result == VK_SUBOPTIMAL_KHR) ) {
                on_window_resize();
                if( result == VK_ERROR_OUT_OF_DATE_KHR ) {
                    return;
                }
        }

        CHECK_RESULT(result);
        current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
    }

    virtual void create_renderpass() = 0;

    virtual void render() = 0;
};
