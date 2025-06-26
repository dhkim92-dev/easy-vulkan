#include "ev-image.h"

using namespace std;
using namespace ev;
using namespace ev::logger;

Image::Image(
    shared_ptr<Device> device,
    VkImageType type,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint32_t depth ,
    uint32_t mip_levels,
    uint32_t array_layers,
    VkImageLayout layout,
    VkSampleCountFlagBits samples,
    VkImageTiling tiling,
    VkImageUsageFlags usage_flags,
    VkImageCreateFlags flags,
    VkSharingMode sharing_mode,
    uint32_t queue_family_count,
    const uint32_t* queue_family_indices,
    const void* p_next
) : device(device), 
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
        Logger::getInstance().info("VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT is set, ensure the image format is compatible with the view format.");
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
    CHECK_RESULT(vkCreateImage(*device, &ci, nullptr, &image));
    Logger::getInstance().debug("Image created successfully.");
}

VkResult Image::bind_memory(shared_ptr<Memory> memory, VkDeviceSize offset) {
    if (!memory) {
        Logger::getInstance().error("Memory is null.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    this->memory = memory;
    VkResult result = vkBindImageMemory(*device, image, *memory, offset);
    if (result != VK_SUCCESS) {
        Logger::getInstance().error("Failed to bind image memory: " + std::to_string(result));
    }
    return result;
}

void Image::destroy() {
    Logger::getInstance().debug("Destroying Image...");
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(*device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    usage_flags = 0;
    Logger::getInstance().debug("Image destroyed successfully.");
}

Image::~Image() {
    destroy();
}