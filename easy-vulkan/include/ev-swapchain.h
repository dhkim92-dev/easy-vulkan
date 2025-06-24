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

    void create_swapchain();
    void create_images();
    void create_image_views();
public:
    VkFormat image_format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    uint32_t image_count = 0;
    uint32_t queue_index = UINT32_MAX;


    Swapchain(
        std::shared_ptr<Instance> instance,
        std::shared_ptr<PhysicalDevice> pdevice,
        std::shared_ptr<Device> device
    );
    ~Swapchain();
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    void create_swapchain(
        VkSurfaceKHR surface,
        uint32_t width = 1280,
        uint32_t height = 720,
        bool enable_vsync = false,
        bool fullscreen = false
    );
    void recreate_swapchain(VkExtent2D new_extent);
    void destroy_swapchain();



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
        return image_views;
    }
    operator VkSwapchainKHR() const {
        return swapchain;
    }
};

}