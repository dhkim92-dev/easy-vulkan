#pragma once

#include "ev-device.h"
#include "ev-command_buffer.h"
#include "ev-logger.h"

namespace ev {

class CommandPool {

private:

    shared_ptr<Device> device;

    VkCommandPool command_pool = VK_NULL_HANDLE;

    VkQueueFlags queue_flags;

    VkCommandPoolCreateFlags flags;

    uint32_t queue_family_index;

public: 

    CommandPool(shared_ptr<Device> device, VkQueueFlags queue_flags, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    ~CommandPool();

    shared_ptr<CommandBuffer> allocate(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    vector<shared_ptr<CommandBuffer>> allocate(size_t nr_commands, VkCommandBufferLevel level);

    void destroy();

    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool(const CommandPool&) = delete;

    uint32_t get_queue_family_index() const {
        return queue_family_index;
    }

    operator VkCommandPool() const {
        return command_pool;
    }

};

}