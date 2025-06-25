#include "ev-buffer.h"
#include "ev-logger.h"

using namespace std;
using namespace ev;
using namespace ev::logger;

Buffer::Buffer(
    std::shared_ptr<Device> device,
    VkDeviceSize size,
    VkBufferUsageFlags usage_flags
) : device(device), size(size), usage_flags(usage_flags) {
    Logger::getInstance().debug("Creating Buffer with size: " + std::to_string(size));
    alignment = device->get_properties().limits.minUniformBufferOffsetAlignment;
    if (!device) {
        Logger::getInstance().error("Invalid device provided for Buffer creation.");
        exit(EXIT_FAILURE);
    }
    Logger::getInstance().debug("Buffer created successfully.");
}

VkResult Buffer::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage_flags
) {
    Logger::getInstance().debug("Creating Vulkan buffer...");
    this->size = size;
    this->usage_flags = usage_flags;
    // this->memory_flags = memory_flags;
    VkBufferCreateInfo buffer_ci = {};
    buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_ci.size = size;
    buffer_ci.usage = usage_flags;
    buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Single queue family 

    return vkCreateBuffer(*device, &buffer_ci, nullptr, &buffer);
}

VkResult Buffer::bind_memory(VkDeviceMemory memory, VkDeviceSize offset) {
    Logger::getInstance().debug("Binding memory to buffer...");
    if (buffer == VK_NULL_HANDLE) {
        Logger::getInstance().error("Buffer is not created yet.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return vkBindBufferMemory(*device, buffer, memory, offset);
}

/**
 * @brief Buffer의 메모리를 맵핑하는 함수
 * @param offset : 맵핑할 메모리의 시작 오프셋,
 * size : 맵핑할 메모리의 크기, VK_WHOLE_SIZE 를 지정하면 전체 메모리를 맵핑한다.
 * @return VkResult : VK_SUCCESS on success, error code on failure
 */
VkResult Buffer::map(VkDeviceSize offset, VkDeviceSize size) {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("Mapping buffer memory...");
    if (buffer == VK_NULL_HANDLE) {
        Logger::getInstance().error("Buffer is not created yet.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (is_mapped) {
        Logger::getInstance().warn("Buffer is already mapped.");
        return VK_SUCCESS;
    }

    VkResult result = vkMapMemory(*device, memory, offset, size, 0, &mapped);
    if (result == VK_SUCCESS) {
        is_mapped = true;
    } else {
        Logger::getInstance().error("Failed to map buffer memory: " + std::to_string(result));
    }
    return result;
}

/** 
 * @brief Buffer의 메모리를 언맵핑하는 함수
 * @return VkResult : VK_SUCCESS on success, error code on failure
*/
VkResult Buffer::unmap() {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("Unmapping buffer memory...");
    if (!is_mapped) {
        Logger::getInstance().warn("Buffer is not mapped, nothing to unmap.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }
    vkUnmapMemory(*device, memory);
    mapped = nullptr;
    is_mapped = false;
    return VK_SUCCESS;
}

VkResult Buffer::invalidate(VkDeviceSize offset, VkDeviceSize size) {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("Invalidating buffer memory...");
    if (!is_mapped) {
        Logger::getInstance().error("Buffer is not mapped, cannot invalidate.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = memory;
    range.offset = offset;
    range.size = size;

    return vkInvalidateMappedMemoryRanges(*device, 1, &range);
}

/**
 * @brief VkBuffer 에 binding 된 메모리가 VK_MEMORY_PROPERTY_HOST_NON_COHERENT_BIT 가 설정된 경우에만 
 * 유효한 동작을 한다. 그렇지 않다면 에러는 발생하지 않지만, 불필요한 호출이 반복된다.
 */
VkResult Buffer::flush( VkDeviceSize offset, VkDeviceSize size ) {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("Flushing entire buffer memory...");
    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = memory;
    range.offset = 0;
    range.size = size;
    return vkFlushMappedMemoryRanges(*device, 1, &range);
}

/**
 * @brief Buffer에 데이터를 쓰는 함수
 * @param data : 쓰고자 하는 데이터의 포인터
 * @param size : 쓰고자 하는 데이터의 크기
 * @return VkResult : VK_SUCCESS on success, error code on failure
 */
VkResult Buffer::write(void* data, VkDeviceSize size) {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("Writing data to buffer...");
    if (!is_mapped) {
        Logger::getInstance().error("Buffer is not mapped, cannot write data.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    if (size > this->size) {
        Logger::getInstance().error("Data size exceeds buffer size.");
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    memcpy(mapped, data, size);
    return VK_SUCCESS;
}

/**
 * @brief Buffer에서 데이터를 읽는 함수, 연동된 메모리가 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 가 설정되어 있을 때만 동작한다.
 * @param data : 읽은 데이터를 저장할 포인터
 * @param size : 읽고자 하는 데이터의 크기
 * @return VkResult : VK_SUCCESS on success, error code on failure
 */
VkResult Buffer::read(void* data, VkDeviceSize size) {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("Reading data from buffer...");
    if (!is_mapped) {
        Logger::getInstance().error("Buffer is not mapped, cannot read data.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    if (size > this->size) {
        Logger::getInstance().error("Data size exceeds buffer size.");
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    memcpy(data, mapped, size);
    return VK_SUCCESS;
}

/**
 * @brief Buffer를 파괴하는 함수
 * 이 함수는 Buffer 객체가 소멸될 때 호출되어야 하며, 
 * 메모리 해제 및 버퍼 파괴를 수행한다.
 */
void Buffer::destroy() {
    Logger::getInstance().debug("Destroying Buffer...");
    if ( is_mapped ) {
        flush();
        invalidate();
        unmap();
        is_mapped = false;
    }

    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(*device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }

    size = 0;
    usage_flags = 0;
    Logger::getInstance().debug("Buffer destroyed successfully.");
}

Buffer::~Buffer() {
    Logger::getInstance().debug("Destroying Buffer...");
    destroy();
    Logger::getInstance().debug("Buffer destroyed successfully.");
}