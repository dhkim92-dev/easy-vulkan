// #include <caasert>
#include <assert.h>
#include "ev-swapchain.h"
#include "ev-logger.h"

using namespace std;
using namespace ev;
using namespace ev::logger;

Swapchain::Swapchain(
    std::shared_ptr<Instance> _instance,
    std::shared_ptr<PhysicalDevice> _pdevice,
    std::shared_ptr<Device> _device
) : instance(std::move(_instance)), pdevice(std::move(_pdevice)), device(std::move(_device)) {
    Logger::getInstance().debug("Creating Swapchain...");
    if (!instance || !pdevice || !device) {
        Logger::getInstance().error("Invalid instance, physical device, or device provided for Swapchain creation.");
        exit(EXIT_FAILURE);
    }
    Logger::getInstance().debug("Swapchain created successfully.");
}

void Swapchain::find_present_queue() {
    Logger::getInstance().debug("Finding present queue for the surface...");
    vector<VkQueueFamilyProperties> queue_family_properties = device->get_queue_family_properties();
    uint32_t queue_count = static_cast<uint32_t>(queue_family_properties.size());

    if (queue_count == 0) {
        Logger::getInstance().error("No queue families available for the physical device.");
        exit(EXIT_FAILURE);
    }

    vector<VkBool32> support_presents(queue_count);
    auto vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(*instance, "vkGetPhysicalDeviceSurfaceSupportKHR"); 
    for (uint32_t i = 0 ; i < queue_count ; ++i ) {
        vkGetPhysicalDeviceSurfaceSupportKHR(*pdevice, i, surface, &support_presents[i]);
    }

    // Find the graphics and present queue indices
    uint32_t graphics_queue = UINT32_MAX;
    uint32_t present_queue = UINT32_MAX;

    for (uint32_t i = 0 ; i < queue_count ; ++i) {
        if ( (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ) {
            if ( graphics_queue == UINT32_MAX ) {
                graphics_queue = i;
            }
            if ( support_presents[i] ) {
                graphics_queue = i;
                present_queue = i; 
                break;             
            }
        }
    }

    if ( present_queue == UINT32_MAX ) {
        for (uint32_t i = 0 ; i < queue_count ; ++i) {
            if ( support_presents[i] == VK_TRUE ) {
                present_queue = i;
                break;
            }
        }
    }

    if ( graphics_queue == UINT32_MAX || present_queue == UINT32_MAX ) {
        Logger::getInstance().error("No suitable graphics or present queue found for the surface.");
        exit(EXIT_FAILURE);
    }

    if ( graphics_queue != present_queue ) {
        Logger::getInstance().warn("Graphics and present queue are different, and it not be supported by this library");
        exit(EXIT_FAILURE);
    }

    queue_index = graphics_queue;
    Logger::getInstance().debug("Present queue found successfully at index: " + std::to_string(queue_index));
}

void Swapchain::find_color_format_and_space() {
    // 스왑체인에서 사용할 이미지 포맷과 색상 공간을 결정한다.
    Logger::getInstance().debug("Finding color format and color space for the surface...");
    auto vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(*instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    uint32_t format_count = 0;
    CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(*pdevice, surface, &format_count, nullptr));
    
    if (format_count == 0) {
        Logger::getInstance().error("No surface formats available for the physical device.");
        exit(EXIT_FAILURE);
    }

    vector<VkSurfaceFormatKHR> surface_formats(format_count);
    CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(*pdevice, surface, &format_count, surface_formats.data()));

    vector<VkFormat> prefered_formats = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_A8B8G8R8_UNORM_PACK32,
    };

    for (auto& available : surface_formats) {
        if ( std::find(prefered_formats.begin(), prefered_formats.end(), available.format) != prefered_formats.end() ) {
            image_format = available.format;
            color_space = available.colorSpace;
            break;
        }
    }

    if (image_format == VK_FORMAT_UNDEFINED ) { 
        Logger::getInstance().error("No suitable image format found for the surface.");
        exit(EXIT_FAILURE);
    }

    Logger::getInstance().debug("Image format and color space determined successfully: " + std::to_string(image_format) + ", " + std::to_string(color_space));
}

void Swapchain::register_surface(VkSurfaceKHR surface) {
    Logger::getInstance().debug("Registering surface for the swapchain...");
    // 스왑체인 객체에서 사용할 surface를 등록한다. 이 때 이미 등록된 Surface가 있다면 에러가 발생한다.
    if (this->surface != VK_NULL_HANDLE) {
        Logger::getInstance().error("Surface already registered, replacing with new surface.");
        exit(EXIT_FAILURE);
    }
    this->surface = surface;
    Logger::getInstance().debug("Surface registered successfully.");
    // Present 모드를 확인하고, Present Queue를 설정한다.
    find_present_queue();
    find_color_format_and_space();
    Logger::getInstance().debug("Surface registration completed successfully.");
}

void Swapchain::create(
    VkSurfaceKHR surface,
    uint32_t& width,
    uint32_t& height,
    bool enable_vsync,
    bool fullscreen
) {
    Logger::getInstance().debug("Creating swapchain with surface...");
    register_surface(surface);
    VkSwapchainKHR old_swapchain = swapchain;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    auto vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(*instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*pdevice, surface, &surface_capabilities));
    VkExtent2D resolution = {width, height};
    if ( surface_capabilities.currentExtent.width == 0xFFFFFFFF ) {
        resolution.height = width;
        resolution.width = height;
    } else {
        resolution = surface_capabilities.currentExtent;
        width = resolution.width;
        height = resolution.height;
    }

    uint32_t present_mode_count = 0;
    auto vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(*instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(*pdevice, surface, &present_mode_count, nullptr));
    if (present_mode_count == 0) {
        Logger::getInstance().error("No present modes available for the surface.");
        exit(EXIT_FAILURE);
    }
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(*pdevice, surface, &present_mode_count, present_modes.data()));  

    // Vsync가 활성화 된 경우 FIFO를 기본으로 이용, 없다면 MAIL_BOX -> IMMEDIATE 순으로 선택
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // Default to FIFO
    if ( !enable_vsync ) {
        for (const auto& mode : present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                present_mode = mode;
                break;
            } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                present_mode = mode;
            }
        }
    }

    // Swapchain Image 수를 설정
    uint32_t target_swapchain_image_count = surface_capabilities.minImageCount + 1;
    if( (surface_capabilities.maxImageCount > 0) && 
        (target_swapchain_image_count > surface_capabilities.maxImageCount) ) {
        target_swapchain_image_count = surface_capabilities.maxImageCount;
    }

    // surface transformation 설정
    VkSurfaceTransformFlagsKHR preferred_transform;
    if ( surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) {
        preferred_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // Use identity transform if available
    } else {
        preferred_transform = surface_capabilities.currentTransform; // Fallback to current transform
    }

    // composite alpha format 결정
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Default to opaque
    std::vector<VkCompositeAlphaFlagBitsKHR> composite_alpha_flags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };
    for (auto& flag : composite_alpha_flags) {
        if ( surface_capabilities.supportedCompositeAlpha & flag ) {
            composite_alpha = flag; // Use the first supported composite alpha format
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = surface;
    swapchain_ci.minImageCount = target_swapchain_image_count;
    swapchain_ci.imageFormat = image_format;
    swapchain_ci.imageColorSpace = color_space;
    swapchain_ci.imageExtent = resolution;
    swapchain_ci.imageArrayLayers = 1; // Single layer for 2D images
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // TODO("다수의 큐를 이용할 경우 변경이 필요")
    swapchain_ci.queueFamilyIndexCount = 0; 
    swapchain_ci.pQueueFamilyIndices = nullptr; 
    swapchain_ci.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preferred_transform);
    swapchain_ci.compositeAlpha = composite_alpha;
    swapchain_ci.presentMode = present_mode;
    swapchain_ci.clipped = VK_TRUE; // Clipping enabled
    swapchain_ci.oldSwapchain = old_swapchain; // Use the old swapchain if

    if ( surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT  ) {
        swapchain_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // Add transfer source usage if supported
    }

    if ( surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ) {
        swapchain_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Add transfer destination usage if supported
    }

    CHECK_RESULT(vkCreateSwapchainKHR(*device, &swapchain_ci, nullptr, &swapchain));

    if ( old_swapchain != VK_NULL_HANDLE ) {
        destroy_swapchain(old_swapchain);
    }

    images.resize(image_count);
    auto vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetInstanceProcAddr(*instance, "vkGetSwapchainImagesKHR");
    CHECK_RESULT(vkGetSwapchainImagesKHR(*device, swapchain, &image_count, images.data()));
    views.resize(image_count);

    for ( size_t i = 0 ; i < images.size() ; ++i ) {
        create_image_view(images[i], views[i]);
    }

    Logger::getInstance().debug("Swapchain created successfully with " + std::to_string(image_count) + " images.");
}

void Swapchain::create_image_view(VkImage& image, VkImageView &view) {
    Logger::getInstance().debug("Creating image view for swapchain image...");
    VkImageViewCreateInfo view_ci = {};
    view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_ci.image = image;
    view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D; // 2D images
    view_ci.format = image_format;
    view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Color aspect
    view_ci.subresourceRange.baseMipLevel = 0;
    view_ci.subresourceRange.levelCount = 1; // Single mip level
    view_ci.subresourceRange.baseArrayLayer = 0;
    view_ci.subresourceRange.layerCount = 1; // Single layer
    CHECK_RESULT(vkCreateImageView(*device, &view_ci, nullptr, &view));
    Logger::getInstance().debug("Image view created successfully for swapchain image.");
}

void Swapchain::acquire_next_image(
    VkSemaphore semaphore,
    uint32_t& image_index,
    VkFence fence
) {
    Logger::getInstance().debug("Acquiring next image from swapchain...");
    auto vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetInstanceProcAddr(*instance, "vkAcquireNextImageKHR");
    CHECK_RESULT(vkAcquireNextImageKHR(*device, swapchain, UINT64_MAX, semaphore, fence, &image_index));
    Logger::getInstance().debug("Next image acquired successfully at index: " + std::to_string(image_index));
}

void Swapchain::destroy_swapchain(VkSwapchainKHR& sc) {
    if (swapchain != VK_NULL_HANDLE) {
        for (VkImageView view : views) {
            vkDestroyImageView(*device, view, nullptr);
        }
        vkDestroySwapchainKHR(*device, swapchain, nullptr);
    } else {
    }
}

void Swapchain::destroy() {
    if (swapchain != VK_NULL_HANDLE) {
        for (auto image_view : views) {
            vkDestroyImageView(*device, image_view, nullptr);
        }
        views.clear();
        images.clear();
        vkDestroySwapchainKHR(*device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
}

Swapchain::~Swapchain() {
    destroy_swapchain(swapchain);
}