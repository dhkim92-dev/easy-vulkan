#include "ev-commandpool.h"

using namespace ev;

CommandPool::CommandPool(shared_ptr<Device> _device, VkQueueFlags queue_flags, VkCommandPoolCreateFlags flags)
    : device(std::move(_device)), 
      queue_flags(queue_flags), 
      flags(flags) {
    if (!this->device) {
        logger::Logger::getInstance().error("Invalid device provided for CommandPool creation.");
        exit(EXIT_FAILURE);
    }

    queue_family_index = this->device->get_queue_index(queue_flags);
    VkCommandPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_ci.queueFamilyIndex = this->queue_family_index;
    pool_ci.flags = flags;
    CHECK_RESULT(vkCreateCommandPool(*this->device, &pool_ci, nullptr, &command_pool));
    logger::Logger::getInstance().debug("CommandPool created successfully.");
}

shared_ptr<CommandBuffer> CommandPool::allocate(VkCommandBufferLevel level) {
    if (command_pool == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Command pool is not created.");
        return nullptr;
    }

    return make_shared<CommandBuffer>(device, *this, level);
}

vector<shared_ptr<CommandBuffer>> CommandPool::allocate(size_t nr_commands, VkCommandBufferLevel level) {
    if (command_pool == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Command pool is not created.");
        return {};
    }
    logger::Logger::getInstance().debug("Allocating " + std::to_string(nr_commands) + " command buffers of level " + std::to_string(level) + ".");

    if (nr_commands == 0) {
        return {};
    }

    // vector<VkCommandBuffer> command_buffers(nr_commands);
    VkCommandBuffer *buffers = new VkCommandBuffer[nr_commands];
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = level;
    alloc_info.commandBufferCount = static_cast<uint32_t>(nr_commands);
    alloc_info.pNext = nullptr; // No additional structures
    VkResult result = vkAllocateCommandBuffers(*device, &alloc_info, buffers);
    CHECK_RESULT(result);
    vector<shared_ptr<CommandBuffer>> command_buffer_objects(nr_commands);
    delete[] buffers;
    return {};

    // return command_buffer_objects;
}

void CommandPool::destroy() {
    if (command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(*device, command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
        logger::Logger::getInstance().debug("CommandPool destroyed successfully.");
    } else {
        logger::Logger::getInstance().debug("CommandPool is already null, no action taken.");
    }
}

CommandPool::~CommandPool() {
    destroy();
}