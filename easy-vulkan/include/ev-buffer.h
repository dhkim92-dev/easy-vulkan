#pragma once

#include <memory>
#include <vector>
#include <cstring>
#include "ev-device.h"
#include "ev-memory.h"

using namespace std;

namespace ev {

/**
 * @brief Buffer class for managing Vulkan buffers.
 */
class Buffer {

private:

    std::shared_ptr<Device> device;

    VkBuffer buffer = VK_NULL_HANDLE;

    VkDeviceMemory memory = VK_NULL_HANDLE;

    VkDeviceSize size = 0;

    VkDeviceSize alignment = 0;

    VkBufferUsageFlags usage_flags = 0;

    VkDescriptorBufferInfo descriptor = {};
    
    void *mapped;

    bool is_mapped = false;

    VkResult create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage_flags
        // VkMemoryPropertyFlags memory_flags
    );

public:

    explicit Buffer(
        std::shared_ptr<Device> device,
        VkDeviceSize size,
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        // VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    ~Buffer();

    VkResult bind_memory(shared_ptr<ev::Memory> memory, VkDeviceSize offset);

    VkResult map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    VkResult unmap();

    VkResult flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    VkResult invalidate(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    VkResult write(void* data, VkDeviceSize size);

    VkResult read(void* data, VkDeviceSize size);

    void destroy();

    VkDescriptorBufferInfo& get_descriptor() {
        descriptor.buffer = buffer;
        return descriptor;
    }

    void* get_mapped_ptr() const {
        return mapped;
    }

    VkDeviceSize get_size() const {
        return size;
    }

    VkDeviceSize get_alignment() const {
        return alignment;
    }

    VkBufferUsageFlags get_usage_flags() const {
        return usage_flags;
    }

    VkMemoryRequirements get_memory_requirements() const {
        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(*device, buffer, &mem_reqs);
        return mem_reqs;
    }

    operator VkBuffer() const {
        return buffer;
    }
};

}
