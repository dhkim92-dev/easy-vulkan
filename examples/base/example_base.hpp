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
        std::filesystem::path bin_path = std::filesystem::canonical(executable_path);
        std::filesystem::path base_path = bin_path.parent_path().parent_path().parent_path();

        shader_path = base_path.parent_path() / "shaders";
        auto build_path = base_path.parent_path();
        resource_path = build_path.parent_path() /  "resources";

        ev_log_info("Executable path set to: %s", executable_path.c_str());
        ev_log_info("Shader path set to: %s", shader_path.string().c_str());
        ev_log_info("Resource path set to: %s", resource_path.string().c_str());
    };

    virtual ~ExampleBase() {
        pre_destroy();

        if (swapchain) {
            swapchain->destroy();
        }

        vkDestroySurfaceKHR(instance->get_instance(), surface, nullptr);
    }

    void run() {
        while(!glfwWindowShouldClose(display.window)) {
            glfwPollEvents();
            render();
        }
        queue->wait_idle();
        device->wait_idle();
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

    std::shared_ptr<ev::MemoryAllocator> memory_allocator;

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

    std::shared_ptr<ev::CommandPool> command_pool;

    std::vector<std::shared_ptr<ev::CommandBuffer>> command_buffers;

    std::vector<std::shared_ptr<ev::Semaphore>> render_completes;

    std::vector<std::shared_ptr<ev::Semaphore>> present_completes;

    std::vector<std::shared_ptr<ev::Fence>> flight_fences;

    uint32_t current_buffer_index = 0;

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
        create_memory_pool();
        create_depth_stencil_buffers();
        create_renderpass();
        create_framebuffers();
        create_commandpool();
        create_commandbuffers();
        setup_sync();
    }

    virtual void create_instance() {
        std::vector<const char*> required_extensions = setup_instance_extensions();
        std::vector<const char*> required_layers = setup_instance_layers();
        
        if (debug) {
            required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            required_layers.push_back("VK_LAYER_KHRONOS_validation");
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

    virtual void init_window(bool fullscreen = true) {
        if (!glfwInit()) {
            ev_log_error("Failed to initialize GLFW");
            exit(EXIT_FAILURE);
        }

        if ( fullscreen ) {
            ev_log_info("Creating fullscreen GLFW window...");
            GLFWmonitor *primary_monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode *vid_mode = glfwGetVideoMode(primary_monitor);
            display.width = vid_mode->width;
            display.height = vid_mode->height;
        }  

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); 
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        display.window = glfwCreateWindow(display.width, display.height, title.c_str(), nullptr, nullptr);
        if (!display.window) {
            ev_log_error("Failed to create GLFW window");
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwSetWindowUserPointer(display.window, this);
        glfwGetFramebufferSize(display.window, (int*)&display.width, (int*)&display.height);
        ev_log_info("GLFW window created with dimensions: %ux%u", display.width, display.height);

        glfwSetFramebufferSizeCallback(display.window, [](GLFWwindow* window, int width, int height) {
            // std::printf("Framebuffer resize detected: width=%d, height=%d\n", width, height);
            ev_log_debug("Framebuffer resize detected: width=%d, height=%d", width, height);
            ExampleBase* app = reinterpret_cast<ExampleBase*>(glfwGetWindowUserPointer(window));
            app->resized = true;
        });

        glfwSetWindowSizeCallback(display.window, [](GLFWwindow* window, int width, int height) {
            // std::printf("Window resize detected: width=%d, height=%d\n", width, height);
            ev_log_debug("Window resize detected: width=%d, height=%d", width, height);
            ExampleBase* app = reinterpret_cast<ExampleBase*>(glfwGetWindowUserPointer(window));
            app->on_window_resize();
        });
    }

    virtual void create_surface() {
        ev_log_debug("Creating Vulkan surface for GLFW window... surface handler current hex : %llu", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(surface)));
        CHECK_RESULT(glfwCreateWindowSurface(*instance, display.window, nullptr, &surface));
        ev_log_debug("surface create complete surface handler current hex : %llu", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(surface)));
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
        ev_log_debug("[Setup Queue Start]");
        queue = std::make_shared<ev::Queue>(device, device->get_queue_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT));
        ev_log_debug("[Setup Queue End]");
    }

    virtual void create_memory_pool() = 0;

    virtual void on_window_resize() {
        if ( !prepared ) {
            ev_log_info("[on_window_resize] Swapchain not prepared yet, skipping resize handling.");
            return;
        }

        prepared = false;
        resized = true;

        int w,h;
        glfwGetFramebufferSize(display.window, &w, &h);
        while (w == 0 || h == 0) {
            glfwGetFramebufferSize(display.window, &w, &h);
            glfwWaitEvents();
        }

        display.width = w;
        display.height = h;

        ev_log_info("[on_window_resize] Waiting for device to be idle before resizing...");
        device->wait_idle(); // 중요, 윈도우 리사이즈 콜백 수행 중에 GPU가 작업 중이면 안되므로 대기 
        ev_log_info("[on_window_resize] Device is now idle. Proceeding with resize...");

        destory_depth_stencil_buffers();
        destroy_framebuffers();
        destroy_swapchain();
        ev_log_info("Recreating swapchain with new dimensions: width=%u, height=%u", display.width, display.height);
        create_swapchain();
        create_depth_stencil_buffers();
        create_framebuffers();
        ev_log_info("Recreation complete.");
        
        ev_log_info("[on_window_resize] Recreating synchronization primitives...");
        destroy_sync();
        ev_log_info("[on_window_resize] Synchronization primitives destroyed.");
        setup_sync();
        ev_log_info("[on_window_resize] Synchronization primitives recreated.");

        prepared = true;
        resized = false;
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
        
        // current_frame_index = 0;
    }

    virtual void destroy_swapchain() {
        ev_log_info("[destroy_swapchain] Destroying swapchain... reference count before reset: %zu", swapchain.use_count());
        swapchain.reset();
        ev_log_info("[destroy_swapchain] Swapchain destroyed. Reference count after reset: %zu", swapchain.use_count());
        // max_frames_in_flight = 2;
    }

    virtual void create_framebuffers() {
        // Basically use color and Depth stencil framebuffer creation.
        // std::printf("renderpass address : %llu\n", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(VkRenderPass(*render_pass))));
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

    virtual void create_commandpool() {
        command_pool = std::make_shared<ev::CommandPool>(device, device->get_queue_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT));
    }

    virtual void create_commandbuffers() {
        command_buffers.resize(max_frames_in_flight);
        for ( uint32_t i = 0 ; i < max_frames_in_flight ; ++i ) {
            command_buffers[i] = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        }
    }

    virtual void destroy_framebuffers() {
        framebuffers.clear();
    }

    virtual void setup_sync() {
        render_completes.resize(max_frames_in_flight);
        present_completes.resize(max_frames_in_flight);
        flight_fences.resize(max_frames_in_flight);

        for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
            render_completes[i] = std::make_shared<ev::Semaphore>(device);
            present_completes[i] = std::make_shared<ev::Semaphore>(device);
            flight_fences[i] = std::make_shared<ev::Fence>(device, VK_FENCE_CREATE_SIGNALED_BIT);
        }
        prepared = true;
        resized = false;
    }

    virtual void destroy_sync() {
        render_completes.clear();
        present_completes.clear();
        flight_fences.clear();
    }

    virtual void prepare_frame(bool wait_fences = false) {
        if ( wait_fences ) {
            ev_log_debug("[prepare_frame] Waiting for fence at index %u to be signaled.", current_buffer_index);
            flight_fences[current_buffer_index]->wait(UINT64_MAX);
            ev_log_debug("[prepare_frame] Fence at index %u signaled, proceeding.", current_buffer_index);
            flight_fences[current_buffer_index]->reset();
        }

        VkResult result = swapchain->acquire_next_image(
            current_frame_index,
            present_completes[current_buffer_index],
            VK_NULL_HANDLE
        );

        if ( (result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR) ) {
            ev_log_debug("[prepare_frame] Swapchain out of date or suboptimal, handling resize...");
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                on_window_resize();
            }
            return;
        } else {
            CHECK_RESULT(result);
        }

        ev_log_debug("[prepare_frame] Acquired image index:  %u", current_frame_index);
    }

    virtual void submit_frame() {
        VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        std::vector<std::shared_ptr<ev::Semaphore>> wait_semaphores = { present_completes[current_buffer_index] };
        std::vector<std::shared_ptr<ev::Semaphore>> signal_semaphores = { render_completes[current_frame_index]  };
        ev_log_debug("[submit_frame] Submitting frame at index: %u", current_buffer_index);

        VkResult result = queue->submit(
            command_buffers[current_buffer_index],
            wait_semaphores,
            signal_semaphores,
            &wait_stage_mask,
            flight_fences[current_buffer_index],
            nullptr
        );
        CHECK_RESULT(result);
        ev_log_debug("[submit_frame] Command buffer submitted successfully for frame index: %u", current_buffer_index);
        result = queue->present(
            swapchain,
            current_frame_index,
            { render_completes[current_frame_index] }
        );

        if ( (result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR) ) {
            ev_log_debug("[submit_frame] Swapchain out of date or suboptimal during present, handling resize...");
            on_window_resize();
            if ( result == VK_ERROR_OUT_OF_DATE_KHR ) {
                return;
            }  
        } else {
            CHECK_RESULT(result);
            ev_log_debug("[submit_frame] Present operation successful for frame index: %u", current_frame_index);
        }

        current_buffer_index = (current_buffer_index + 1) % max_frames_in_flight;
    }

    virtual void create_renderpass() = 0;

    virtual void render() = 0;
};
