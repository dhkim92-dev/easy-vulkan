#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "ev-memory.h"
#include "ev-device.h"
#include "ev-logger.h"

using namespace std;

namespace ev {

class Image {

private: 

    shared_ptr<Device> device;

    shared_ptr<Memory> memory;

    VkImage image = VK_NULL_HANDLE;

    VkImageType type;

    VkExtent3D extent = {0, 0, 0};

    VkFormat format = VK_FORMAT_UNDEFINED;

    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageUsageFlags usage_flags = 0;

    uint32_t mip_levels = 1;

    uint32_t array_layers = 1;
    
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;

    uint32_t queue_family_count = 0;

    const uint32_t* queue_family_indices = nullptr;

    const void* p_next = nullptr;

public:

    explicit Image(
        shared_ptr<Device> device,
        VkImageType type,
        VkFormat format,
        uint32_t width,
        uint32_t height,
        uint32_t depth = 1,
        uint32_t mip_levels = 1,
        uint32_t array_layers = 1,
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
        VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VkImageCreateFlags flags = 0,
        VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
        uint32_t queue_family_count = 0,
        const uint32_t* queue_family_indices = nullptr,
        const void* p_next = nullptr
    );

    ~Image();

    void destroy();

    VkResult bind_memory(shared_ptr<Memory> memory, VkDeviceSize offset = 0);

    VkFormat get_format() {
        return this->format;
    }

    VkImageUsageFlags get_image_usage() {
        return this->usage_flags;
    }

    VkImageTiling get_tiling() {
        return this->tiling;
    }

    uint32_t get_mip_levels() {
        return this->mip_levels;
    }

    uint32_t get_array_layers() {
        return this->array_layers;
    }

    VkImageLayout& get_layout() {
        return layout;
    }

    VkSampleCountFlagBits get_samples() {
        return samples;
    }

    VkMemoryRequirements get_memory_requirements() const {
        VkMemoryRequirements memory_requirements = {};
        vkGetImageMemoryRequirements(*device, image, &memory_requirements);
        return memory_requirements;
    }

    VkExtent3D get_extent() {
        return extent;
    }

    operator VkDeviceMemory() const {
        return *memory;
    }

    operator VkImage() const {
        return image;
    }
};

}