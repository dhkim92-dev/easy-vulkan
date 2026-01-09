#include "ev-memory.h"

using namespace ev;

Memory::Memory(
    std::shared_ptr<Device> _device,
    VkDeviceSize size,
    VkMemoryPropertyFlags memory_property_flags,
    VkMemoryRequirements memory_requirements,
    VkMemoryAllocateFlagsInfoKHR* alloc_flags_info
) : device(std::move(_device)), size(size), memory_property_flags(memory_property_flags) {
    ev_log_debug("[ev::Memory::Memory] Creating Memory with size: %llu", static_cast<unsigned long long>(size));
    if (!device) {
        ev_log_error("[ev::Memory::Memory] Invalid device provided for Memory creation");
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
    ev_log_debug("[ev::Memory::Memory] Memory created successfully.");
}

Memory::Memory(
    std::shared_ptr<ev::Device> _device,
    uint32_t memory_type_index,
    VkDeviceSize size
) : device(std::move(_device)), memory_type_index(memory_type_index), size(size) {
    if (!device) {
        ev_log_error("[ev::Memory::Memory] Invalid device provided for Memory creation");
        exit(EXIT_FAILURE);
    }
    ev_log_debug("[ev::Memory::Memory] Creating Memory with size: %llu and memory type index: %u", static_cast<unsigned long long>(size), memory_type_index);
    
    ev_log_debug("[ev::Memory::Memory] Memory created successfully.");
}

VkResult Memory::allocate() {
    ev_log_info("[ev::Memory::allocate] Allocating memory with size: %llu", static_cast<unsigned long long>(size));
    if (memory != VK_NULL_HANDLE) {
        ev_log_warn("[ev::Memory::allocate] Memory already allocated, skipping allocation.");
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
        ev_log_info("[ev::Memory::destroy] Destroying memory with handle: %llu", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(memory)));
        vkFreeMemory(*device, memory, nullptr);
        memory = VK_NULL_HANDLE;
        ev_log_info("[ev::Memory::destroy] Memory freed successfully.");
    }
    size = 0;
    memory_property_flags = 0;
}

Memory::~Memory() {
    destroy();
}