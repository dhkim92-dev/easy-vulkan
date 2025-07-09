#include "ev-memory.h"

using namespace ev;
using namespace ev::logger;

Memory::Memory(
    std::shared_ptr<Device> _device,
    VkDeviceSize size,
    VkMemoryPropertyFlags memory_property_flags,
    VkMemoryRequirements memory_requirements,
    VkMemoryAllocateFlagsInfoKHR* alloc_flags_info
) : device(std::move(_device)), size(size), memory_property_flags(memory_property_flags) {
    Logger::getInstance().debug("[ev::Memory::Memory] Creating Memory with size: " + std::to_string(size));
    if (!device) {
        Logger::getInstance().error("[ev::Memory::Memory] Invalid device provided for Memory creation");
        exit(EXIT_FAILURE);
    }
    VkMemoryAllocateInfo mem_ai = {};
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.allocationSize = memory_requirements.size;
    mem_ai.memoryTypeIndex = device->get_memory_type_index(
        memory_requirements.memoryTypeBits,
        memory_property_flags,
        nullptr
    );
    mem_ai.pNext = alloc_flags_info;
    CHECK_RESULT(vkAllocateMemory(*device, &mem_ai, nullptr, &memory));
    Logger::getInstance().debug("[ev::Memory::Memory] Memory created successfully.");
}

Memory::Memory(
    std::shared_ptr<ev::Device> _device,
    uint32_t memory_type_index,
    VkDeviceSize size
) : device(std::move(_device)), memory_type_index(memory_type_index), size(size) {
    if (!device) {
        Logger::getInstance().error("[ev::Memory::Memory] Invalid device provided for Memory creation");
        exit(EXIT_FAILURE);
    }
    logger::Logger::getInstance().debug("[ev::Memory::Memory] Creating Memory with size: " + std::to_string(size) + " and memory type index: " + std::to_string(memory_type_index));
    
    Logger::getInstance().debug("[ev::Memory::Memory] Memory created successfully.");
}

VkResult Memory::allocate() {
    Logger::getInstance().debug("[ev::Memory::allocate] Allocating memory with size: " + std::to_string(size));
    if (memory != VK_NULL_HANDLE) {
        Logger::getInstance().warn("[ev::Memory::allocate] Memory already allocated, skipping allocation.");
        return VK_SUCCESS;
    }
    VkMemoryAllocateInfo mem_ai = {};
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.allocationSize = size;
    mem_ai.memoryTypeIndex = memory_type_index;
    mem_ai.pNext = nullptr;
    return vkAllocateMemory(*device, &mem_ai, nullptr, &memory);
}

void Memory::destroy() {
    if (memory != VK_NULL_HANDLE) {
        Logger::getInstance().debug("[ev::Memory::destroy] Destroying memory with handle: " + std::to_string(reinterpret_cast<uintptr_t>(memory)));
        vkFreeMemory(*device, memory, nullptr);
        memory = VK_NULL_HANDLE;
        Logger::getInstance().debug("[ev::Memory::destroy] Memory freed successfully.");
    }
    size = 0;
    memory_property_flags = 0;
}

Memory::~Memory() {
    destroy();
}