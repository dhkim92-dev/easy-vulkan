#pragma once

#include "ev-device.h"

namespace ev {

class CommandBuffer {

private:

    shared_ptr<Device> device;

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;

    VkCommandPool command_pool = VK_NULL_HANDLE;

    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

public:

    explicit CommandBuffer(shared_ptr<Device> _device, VkCommandPool command_pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    explicit CommandBuffer(shared_ptr<Device> _device, VkCommandPool command_pool, VkCommandBuffer command_buffer); 

    CommandBuffer& operator=(const CommandBuffer&) = delete;

    CommandBuffer(const CommandBuffer&) = delete;

    ~CommandBuffer();

    void destroy();

    VkResult begin(VkCommandBufferUsageFlags flags = 0);

    VkResult end();

    VkResult reset(VkCommandBufferResetFlags flags = 0);

    operator VkCommandBuffer() const {
        return command_buffer;
    }
};

}