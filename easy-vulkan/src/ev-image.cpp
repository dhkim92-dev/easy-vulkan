#include "ev-image.h"
#include "ev-command_pool.h"
#include <sstream>

using namespace std;
using namespace ev;

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
        ev_log_info("[ev::Image] VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT is set, ensure the image format is compatible with the view format.");
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
        vkGetImageMemoryRequirements(*device, image, &memory_requirements);
    } 

    if (result != VK_SUCCESS) {
        exit(EXIT_FAILURE);
    }
    std::stringstream ss;
    ss << std::hex << reinterpret_cast<uintptr_t>(image);
}

VkResult Image::bind_memory(shared_ptr<Memory> memory, VkDeviceSize offset) {
    if (!memory) {
        ev_log_error("[ev::Image] Memory is null.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    this->memory = memory;
    VkResult result = vkBindImageMemory(*device, image, *memory, offset);
    if (result != VK_SUCCESS) {
        ev_log_error("[ev::Image] Failed to bind image memory: %d", result);
    }
    return result;
}

VkResult Image::bind_memory(std::shared_ptr<MemoryBlockMetadata> metadata) {
    if (!metadata || !metadata->get_memory()) {
        ev_log_error("[ev::Image] Invalid memory block metadata provided for binding.");
        return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    }
    this->metadata = metadata;
    return bind_memory(metadata->get_memory(), metadata->get_offset());
}

VkResult Image::transient_layout(VkImageLayout new_layout) {
    if (layout == new_layout) {
        ev_log_debug("[ev::Image] Image is already in the desired layout.");
        return VK_SUCCESS;
    }
    layout = new_layout; // Update the layout after the transition
    ev_log_debug("[ev::Image] Image layout transitioned to %d", new_layout);
    
    return VK_SUCCESS;
}

/**
 * @brief 이미지에 할당된 메모리를 매핑합니다.
 * @param offset 매핑을 시작할 오프셋입니다. 실제 메모리의 오프셋이 아닌 사용하는 이미지의 상대 오프셋 입니다.(항상 0부터 시작)
 * @param size 매핑할 메모리의 크기입니다. 기본값은 VK_WHOLE_SIZE로 전체 메모리를 매핑합니다.
 * @return VK_SUCCESS on success, or an error code on failure.
 */
VkResult Image::map(VkDeviceSize offset, VkDeviceSize size) {
    if (!memory) {
        ev_log_error("[ev::Image] Memory is not bound to the image.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (mapped_ptr) {
        ev_log_warn("[ev::Image] Image is already mapped, unmapping first.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkDeviceSize global_offset = metadata ? metadata->get_offset() + offset : offset;

    if ( size == VK_WHOLE_SIZE ) {
        if (metadata) {
            size = metadata->get_size() - global_offset;
        } else {
            size = memory->get_size() - global_offset;
        }
    }

    VkResult result = vkMapMemory(
        *device, 
        *memory, offset, size, 0, &mapped_ptr);

    if ( result == VK_SUCCESS ) {
        mapped_offset = global_offset;
        ev_log_debug("[ev::Image] Image memory mapped successfully at offset: %llu, size: %llu", mapped_offset, size);
    } else {
        ev_log_error("[ev::Image] Failed to map image memory: %d", result);
        mapped_ptr = nullptr; // Reset mapped pointer on failure
        mapped_offset = -1;
    }

    return result;
}

VkResult Image::write(
    const void* data,
    VkDeviceSize size
) {
    if (!mapped_ptr) {
        ev_log_error("[ev::Image] Image is not mapped, cannot write data.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    if ( !metadata ) {
        ev_log_error("[ev::Image] Memory block metadata is not set, cannot write data.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (size == VK_WHOLE_SIZE) {
        size = metadata->get_size() - mapped_offset; // Use the size of the memory block minus the offset
    } else if (size > (metadata->get_size() - mapped_offset)) {
        ev_log_error("[ev::Image] Write size exceeds available memory size.");
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    memcpy(mapped_ptr, data, size);
    return VK_SUCCESS;
}

VkResult Image::read(
    void* data,
    VkDeviceSize size
) {
    if (!mapped_ptr) {
        ev_log_error("[ev::Image] Image is not mapped, cannot read data.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    if ( !metadata ) {
        ev_log_error("[ev::Image] Memory block metadata is not set, cannot read data.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (size == VK_WHOLE_SIZE) {
        size = metadata->get_size() - mapped_offset; // Use the size of the memory block minus the offset
    } else if (size > (metadata->get_size() - mapped_offset)) {
        ev_log_error("[ev::Image] Read size exceeds available memory size.");
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    memcpy(data, mapped_ptr, size);
    return VK_SUCCESS;
}

VkResult Image::flush() {
    if (!mapped_ptr) {
        ev_log_error("[ev::Image] Image is not mapped, cannot flush memory.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }
    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = *memory;
    range.offset = mapped_offset;
    range.size = metadata ? metadata->get_size() - mapped_offset : memory->get_size() - mapped_offset;
    ev_log_debug("[ev::Image] Flushing image memory with offset: %llu, size: %llu", range.offset, range.size);
    return vkFlushMappedMemoryRanges(*device, 1, &range);
}

VkResult Image::unmap() {
    if (!mapped_ptr) {
        ev_log_error("[ev::Image] Image is not mapped, cannot unmap.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    vkUnmapMemory(*device, *memory);
    mapped_ptr = nullptr;
    mapped_offset = -1;
    return VK_SUCCESS;
}

void Image::destroy() {
    ev_log_info("[ev::Image] Destroying Image...");
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(*device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    usage_flags = 0;
    ev_log_info("[ev::Image] Image destroyed successfully.");
}

Image::~Image() {
    destroy();
}