#include "ev-swapchain.h"

using namespace ev;

Swapchain::Swapchain(
    std::shared_ptr<Instance> instance,
    std::shared_ptr<PhysicalDevice> pdevice,
    std::shared_ptr<Device> device
) : instance(instance), pdevice(pdevice), device(device) {
    if (!instance || !pdevice || !device) {
        throw std::runtime_error("Invalid instance, physical device, or device provided.");
    }
}

void Swapchain::create_swapchain(
    VkSurfaceKHR surface,
    uint32_t width,
    uint32_t height,
    bool enable_vsync,
    bool fullscreen
) {
    this->surface = surface;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*pdevice, surface, &capabilities);

    image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    VkSurfaceFormatKHR surface_format;
    std::vector<VkSurfaceFormatKHR> formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*pdevice, surface, &formats);
    
    if (formats.empty()) {
        throw std::runtime_error("No supported surface formats found.");
    }

    // Choose the first format that matches the desired color space
    for (const auto& format : formats) {
        if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = format;
            break;
        }
    }

    image_format = surface_format.format;
    color_space = surface_format.colorSpace;

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // Default to FIFO
    if (enable_vsync) {
        present_mode = VK_PRESENT_MODE_FIFO_KHR; // VSync enabled
    } else {
        present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR; // VSync disabled
    }

    VkExtent2D extent = {width, height};
    
    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = surface;
    swapchain_ci.minImageCount = image_count;
    swapchain_ci.imageFormat = image_format;
    swapchain_ci.imageColorSpace = color_space;
    swapchain_ci.imageExtent = extent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    queue_index = device->get_queue_index(VK_QUEUE_GRAPHICS_BIT);
    
    uint32_t queue_family_indices[] = {queue_index};
    
    if (queue_index != UINT32_MAX) {
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Single queue family
    } else {
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Multiple queue families
        swapchain_ci.queueFamilyIndexCount = 1;
        swapchain_ci.pQueueFamilyIndices = queue_family_indices;
    }
    swapchain_ci.preTransform = capabilities.currentTransform;
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    swapchain_ci.presentMode = present_mode;
    swapchain_ci.clipped = VK_TRUE;
    swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
    VkResult result = vkCreateSwapchainKHR(*device, &swapchain_ci, nullptr, &swapchain);
}

void Swapchain::destroy_swapchain() {
    if (swapchain != VK_NULL_HANDLE) {
        for (auto image_view : image_views) {
            vkDestroyImageView(*device, image_view, nullptr);
        }
        image_views.clear();
        images.clear();
        vkDestroySwapchainKHR(*device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
}

Swapchain::~Swapchain() {
    destroy_swapchain();
}