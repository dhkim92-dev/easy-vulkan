#include "ev-buffer.h"
#include "ev-logger.h"

using namespace std;
using namespace ev;
using namespace ev::logger;

Buffer::Buffer(
    std::shared_ptr<Device> _device,
    VkDeviceSize size,
    VkBufferUsageFlags usage_flags
) : device(std::move(_device)), size(size), usage_flags(usage_flags) {
    Logger::getInstance().debug("[ev::Buffer::Buffer] Creating Buffer with size: " + std::to_string(size));
    alignment = device->get_properties().limits.minUniformBufferOffsetAlignment;
    if (!device) {
        Logger::getInstance().error("[ev::Buffer::Buffer] Invalid device provided for Buffer creation.");
        exit(EXIT_FAILURE);
    }
    VkResult result = create_buffer(size, usage_flags);
    Logger::getInstance().debug("[ev::Buffer::Buffer] Buffer created successfully.");
}

VkResult Buffer::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage_flags
) {
    Logger::getInstance().debug("[ev::Buffer::create_buffer] Creating Vulkan buffer...");
    this->size = size;
    this->usage_flags = usage_flags;
    // this->memory_flags = memory_flags;
    VkBufferCreateInfo buffer_ci = {};
    buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_ci.size = size;
    buffer_ci.usage = usage_flags;
    buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Single queue family 

    VkResult result = vkCreateBuffer(*device, &buffer_ci, nullptr, &buffer);

    if (result == VK_SUCCESS) {
        vkGetBufferMemoryRequirements(*device, buffer, &memory_requirements);
    }

    return result;
}

VkResult Buffer::bind_memory(shared_ptr<ev::Memory> memory, VkDeviceSize offset, VkDeviceSize size) {
    Logger::getInstance().debug("[ev::Buffer::bind_memory] Binding buffer : " + std::to_string(reinterpret_cast<uintptr_t>(buffer)) + " to memory: " + std::to_string(reinterpret_cast<uintptr_t>(VkDeviceMemory(*memory))) + " with offset: " + std::to_string(offset));
    VkBindBufferMemoryInfo bind_info = {};
    bind_info.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
    bind_info.buffer = buffer;
    bind_info.memory = *memory;
    bind_info.memoryOffset = offset;
    bind_info.pNext = nullptr;
    this->offset = offset;
    this->allocated_size = size;
    VkResult result = vkBindBufferMemory(*device, buffer, *memory, offset);
    if (result == VK_SUCCESS) {
        this->memory = *memory;
        Logger::getInstance().debug("[ev::Buffer::bind_memory] Buffer memory bound successfully offset : " + std::to_string(offset));
    }  
    return result;
}

VkResult Buffer::bind_memory(std::shared_ptr<ev::MemoryBlockMetadata> block_metadata) {
    Logger::getInstance().debug("[ev::Buffer::bind_memory] Binding buffer : " + std::to_string(reinterpret_cast<uintptr_t>(buffer)) + " to memory block metadata: " + std::to_string(reinterpret_cast<uintptr_t>(block_metadata.get())));
    if (!block_metadata || !block_metadata->get_memory()) {
        Logger::getInstance().error("[ev::Buffer::bind_memory] Invalid memory block metadata provided for binding.");
        return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    }
    this->pool_block_metadata = block_metadata;
    this->offset = block_metadata->get_offset();
    this->allocated_size = block_metadata->get_size();
    Logger::Logger::getInstance().debug("[ev::Buffer::bind_memory] Binding buffer to memory block metadata with offset: " + std::to_string(this->offset) + ", size: " + std::to_string(this->allocated_size));
    return bind_memory(block_metadata->get_memory(), block_metadata->get_offset(), block_metadata->get_size());
}

/**
 * @brief Buffer의 메모리를 맵핑하는 함수
 * @param offset : 맵핑할 메모리의 시작 오프셋,
 * size : 맵핑할 메모리의 크기, VK_WHOLE_SIZE 를 지정하면 전체 메모리를 맵핑한다.
 * @return VkResult : VK_SUCCESS on success, error code on failure
 */

VkResult Buffer::map(VkDeviceSize size) {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("[ev::Buffer::map] Mapping buffer memory with size : " + std::to_string(this->allocated_size));
    if (buffer == VK_NULL_HANDLE) {
        Logger::getInstance().error("[ev::Buffer::map] Buffer is not created yet.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (is_mapped) {
        Logger::getInstance().warn("[ev::Buffer::map] Buffer is already mapped.");
        return VK_SUCCESS;
    }

    if ( size == VK_WHOLE_SIZE ) {
        size = this->allocated_size;
    } else if ( size > this->allocated_size ) {
        Logger::getInstance().error("[ev::Buffer::map] Size exceeds buffer size.");
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }
    VkResult result = vkMapMemory(*device, memory, offset, size, 0, &mapped);
    if (result == VK_SUCCESS) {
        Logger::getInstance().debug("[ev::Buffer::map] Buffer memory mapped successfully.");
        is_mapped = true;
    } else {
        Logger::getInstance().error("[ev::Buffer::map] Failed to map buffer memory: " + std::to_string(result));
    }
    return result;
}

/** 
 * @brief Buffer의 메모리를 언맵핑하는 함수
 * @return VkResult : VK_SUCCESS on success, error code on failure
*/
VkResult Buffer::unmap() {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("[ev::Buffer::unmap] Unmapping buffer memory...");
    if (!is_mapped) {
        Logger::getInstance().warn("[ev::Buffer::unmap] Buffer is not mapped, nothing to unmap.");
        is_mapped = false;
        return VK_ERROR_MEMORY_MAP_FAILED;
    }
    vkUnmapMemory(*device, memory);
    mapped = nullptr;
    is_mapped = false;
    Logger::getInstance().debug("[ev::Buffer::unmap] Buffer memory unmapped successfully.");
    return VK_SUCCESS;
}

VkResult Buffer::invalidate(VkDeviceSize offset, VkDeviceSize size) {
    // TODO: 추추 Memory Object 구현 후 바인딩 할 때 memory_property_flags 를 확인하여 처리해야한다.
    Logger::getInstance().debug("[ev::Buffer::invalidate] Invalidating buffer memory...");
    if (!is_mapped) {
        Logger::getInstance().error("[ev::Buffer::invalidate] Buffer is not mapped, cannot invalidate.");
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
    Logger::getInstance().debug("[ev::Buffer::flush] Flushing entire buffer memory...");
    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = memory;
    if ( offset == 0 ) {
        offset = this->offset; // Use the offset of the buffer if not specified
    }
    range.offset = this->offset;
    if ( size == VK_WHOLE_SIZE ) {
        size = this->allocated_size; // Use the allocated size of the buffer if not specified
    } else if ( size > this->allocated_size ) {
        Logger::getInstance().error("[ev::Buffer::flush] Size exceeds buffer size.");
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    } else if ( offset + size % device->get_physical_device()->get_properties().limits.nonCoherentAtomSize != 0) {
        Logger::getInstance().warn("[ev::Buffer::flush] Offset and size are not aligned to non-coherent atom size, flushing may not be effective.");
        auto non_coherent_atom_size = device->get_physical_device()->get_properties().limits.nonCoherentAtomSize;
        range.size = (size + non_coherent_atom_size - 1) / non_coherent_atom_size * non_coherent_atom_size; // Align size to non-coherent atom size
    } else {
        range.size = size; // Use the specified size
    }

    logger::Logger::getInstance().debug("[ev::Buffer::flush] Flushing buffer memory with offset: " + std::to_string(range.offset) + ", size: " + std::to_string(range.size));
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
    Logger::getInstance().debug("[ev::Buffer::write] Writing data to buffer...");
    if (!is_mapped) {
        Logger::getInstance().error("[ev::Buffer::write] Buffer is not mapped, cannot write data.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    if (size > this->size) {
        Logger::getInstance().error("[ev::Buffer::write] Data size exceeds buffer size.");
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
    Logger::getInstance().debug("[ev::Buffer::read] Reading data from buffer...");
    if (!is_mapped) {
        Logger::getInstance().error("[ev::Buffer::read] Buffer is not mapped, cannot read data.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    if (size > this->size) {
        Logger::getInstance().error("[ev::Buffer::read] Data size exceeds buffer size.");
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
    if ( is_mapped ) {
        // flush();
        // invalidate();
        unmap();
        is_mapped = false;
    }

    if ( pool_block_metadata ) {
        pool_block_metadata.reset();
    }

    if (buffer != VK_NULL_HANDLE) {
        ev::logger::Logger::getInstance().debug("[ev::Buffer::destroy] Destroying buffer with handle: " + std::to_string(reinterpret_cast<uintptr_t>(buffer)));
        vkDestroyBuffer(*device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }

    size = 0;
    usage_flags = 0;
}

Buffer::~Buffer() {
    destroy();
}