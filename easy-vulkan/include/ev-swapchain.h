#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "ev-instance.h"
#include "ev-logger.h"
#include "ev-utility.h"
#include "ev-device.h"

namespace ev {

class Swapchain {

private:
    std::shared_ptr<Instance> instance = nullptr;
    std::shared_ptr<PhysicalDevice> pdevice = nullptr;
    std::shared_ptr<Device> device = nullptr;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkFormat image_format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    uint32_t image_count = 0;
    uint32_t queue_index = UINT32_MAX;

    void register_surface(VkSurfaceKHR surface);
    void find_present_queue();
    void find_color_format_and_space();
    void create_image_view(VkImage& image, VkImageView &view);
    void destroy_swapchain(VkSwapchainKHR& sc);

public:
    explicit Swapchain(
        std::shared_ptr<Instance> instance,
        std::shared_ptr<PhysicalDevice> pdevice,
        std::shared_ptr<Device> device
    );
    ~Swapchain();
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // Queue에서 이 역할을 하는 것이 맞지 않을지?
    // void submit_frame(
    //     VkQueue queue,
    //     VkSemaphore wait_semaphore = VK_NULL_HANDLE,
    //     VkSemaphore signal_semaphore = VK_NULL_HANDLE,
    //     VkFence fence = VK_NULL_HANDLE
    // );

    void acquire_next_image(
        VkSemaphore semaphore,
        uint32_t& image_index,
        VkFence fence = VK_NULL_HANDLE
    );
    

    void create(
        VkSurfaceKHR surface,
        uint32_t& width,
        uint32_t& height,
        bool enable_vsync = false,
        bool fullscreen = false
    );

    void destroy();

    operator VkSwapchainKHR() const {
        return swapchain;
    }

    VkSurfaceKHR get_surface() const {
        return surface;
    }

    VkFormat get_image_format() const {
        return image_format;
    }

    const std::vector<VkImage>& get_images() const {
        return images;
    }

    const std::vector<VkImageView>& get_image_views() const {
        return views;
    }
};

}