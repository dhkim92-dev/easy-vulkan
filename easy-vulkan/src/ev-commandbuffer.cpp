#include "ev-commandbuffer.h"

using namespace std;
using namespace ev;

/**
 * @brief CommandBuffer 생성자
 * @param _device : Device 객체의 shared_ptr
 * @param command_pool : CommandPool 객체의 VkCommandPool 핸들
 * @param level : CommandBuffer의 레벨 (기본값: VK_COMMAND_BUFFER_LEVEL_PRIMARY)
 * 이 생성자는 CommandPool을 이용하여 하나의 커맨드 버퍼를 할당하는 경우 사용합니다.
 */
CommandBuffer::CommandBuffer(shared_ptr<Device> _device, VkCommandPool command_pool, VkCommandBufferLevel level)
    : device(std::move(_device)), command_pool(command_pool), level(level) {
    if (!device) {
        logger::Logger::getInstance().error("Invalid device provided for CommandBuffer creation.");
        exit(EXIT_FAILURE);
    }
    
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = level;
    alloc_info.commandBufferCount = 1;

    CHECK_RESULT(vkAllocateCommandBuffers(*device, &alloc_info, &command_buffer));
}

/**
 * @brief CommandBuffer 생성자
 * @param _device : Device 객체의 shared_ptr
 * @param command_pool : CommandPool 객체의 VkCommandPool 핸들
 * @param command_buffer : 이미 할당된 VkCommandBuffer 핸들
 * 이 생성자는 한번에 대량의 CommandPool을 이용하여 VkCommandBuffer를 할당하는 경우 사용합니다.
 */
CommandBuffer::CommandBuffer(shared_ptr<Device> _device, VkCommandPool command_pool, VkCommandBuffer command_buffer)
: device(std::move(_device)), command_pool(command_pool), command_buffer(command_buffer) {
    if (!device) {
        logger::Logger::getInstance().error("Invalid device provided for CommandBuffer creation.");
        exit(EXIT_FAILURE);
    }
    
    if (command_buffer == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Invalid command buffer handle provided.");
        exit(EXIT_FAILURE);
    }
}

VkResult CommandBuffer::begin(VkCommandBufferUsageFlags flags) {
    if (command_buffer == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Command buffer is not allocated.");
        return VK_SUCCESS;
    }
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = flags;
    return vkBeginCommandBuffer(command_buffer, &begin_info);
}

VkResult CommandBuffer::reset(VkCommandBufferResetFlags flags) {
    if (command_buffer == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Command buffer is not allocated.");
        return VK_SUCCESS;
    }
    return vkResetCommandBuffer(command_buffer, flags);
}

VkResult CommandBuffer::end() {
    if (command_buffer == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Command buffer is not allocated.");
        return VK_SUCCESS;
    }

    return vkEndCommandBuffer(command_buffer);
}

void CommandBuffer::destroy() {
    if (command_buffer != VK_NULL_HANDLE) {
        logger::Logger::getInstance().debug("Destroying CommandBuffer...");
        vkFreeCommandBuffers(*device, command_pool, 1, &command_buffer);
        command_buffer = VK_NULL_HANDLE;
    }
}

CommandBuffer::~CommandBuffer() {
    destroy();
}