#include "ev-image.h"
#include "ev-command_pool.h"
#include <sstream>

using namespace std;
using namespace ev;
using namespace ev::logger;


Image::Image(
    shared_ptr<Device> _device,
    VkImageType type,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint32_t depth ,
    uint32_t mip_levels,
    uint32_t array_layers,
    VkImageUsageFlags usage_flags,
    VkImageLayout layout,
    VkSampleCountFlagBits samples,
    VkImageTiling tiling,
    VkImageCreateFlags flags,
    VkSharingMode sharing_mode,
    uint32_t queue_family_count,
    const uint32_t* queue_family_indices,
    const void* p_next
) : device(std::move(_device)), 
    type(type),
    extent({width, height, depth}),
    format(format), 
    layout(layout), 
    usage_flags(usage_flags),
    mip_levels(mip_levels),
    array_layers(array_layers),
    samples(samples),
    tiling(tiling),
    sharing_mode(sharing_mode),
    queue_family_count(queue_family_count),
    queue_family_indices(queue_family_indices),
    flags(flags),
    p_next(p_next) {
    if( flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT ) {
        Logger::getInstance().info("[ev::Image] VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT is set, ensure the image format is compatible with the view format.");
    }
    VkImageCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = type; // 2D image
    ci.format = format; // Common format
    ci.extent.width = width;
    ci.extent.height = height;
    ci.extent.depth = depth,
    ci.mipLevels = mip_levels; // No mipmaps
    ci.arrayLayers = array_layers; // Single layer
    ci.samples = samples; // No multisampling
    ci.tiling = tiling; // Optimal tiling for performance
    ci.usage = usage_flags; // Usage flags
    ci.sharingMode = sharing_mode;
    ci.initialLayout = layout; // Initial layout
    ci.flags = flags;// No special flags
    ci.pNext = p_next; // No additional structures
    VkResult result = vkCreateImage(*device, &ci, nullptr, &image);

    if ( result == VK_SUCCESS ) {
        Logger::getInstance().debug("[ev::Image] Image created successfully with handle " + std::to_string(reinterpret_cast<uintptr_t>(image)));
        vkGetImageMemoryRequirements(*device, image, &memory_requirements);
    } 

    if (result != VK_SUCCESS) {
        Logger::getInstance().error("[ev::Image] Failed to create image: " + std::to_string(result));
        exit(EXIT_FAILURE);
    }
    std::stringstream ss;
    ss << std::hex << reinterpret_cast<uintptr_t>(image);
    Logger::getInstance().debug("[ev::Image] Image created, handle: 0x" + ss.str() );
}

VkResult Image::bind_memory(shared_ptr<Memory> memory, VkDeviceSize offset) {
    if (!memory) {
        Logger::getInstance().error("[ev::Image] Memory is null.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    this->memory = memory;
    VkResult result = vkBindImageMemory(*device, image, *memory, offset);
    if (result != VK_SUCCESS) {
        Logger::getInstance().error("[ev::Image] Failed to bind image memory: " + std::to_string(result));
    }
    return result;
}

VkResult Image::transient_layout(VkImageLayout new_layout) {
    if (layout == new_layout) {
        Logger::getInstance().debug("[ev::Image] Image is already in the desired layout.");
        return VK_SUCCESS;
    }
    layout = new_layout; // Update the layout after the transition
    Logger::getInstance().debug("[ev::Image] Image layout transitioned to " + std::to_string(new_layout));
    
    return VK_SUCCESS;
}

void Image::destroy() {
    Logger::getInstance().info("[ev::Image] Destroying Image...");
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(*device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    usage_flags = 0;
    Logger::getInstance().info("[ev::Image] Image destroyed successfully.");
}

Image::~Image() {
    destroy();
}