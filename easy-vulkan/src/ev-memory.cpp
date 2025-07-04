
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
    Logger::getInstance().debug("Creating Memory with size: " + std::to_string(size));
    if (!device) {
        Logger::getInstance().error("Invalid device provided for Memory creation");
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
    Logger::getInstance().debug("Memory created successfully.");
}

void Memory::destroy() {
    Logger::getInstance().debug("[Memory::destroy] Destroying memory with handle: " + std::to_string(reinterpret_cast<uintptr_t>(memory)));
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(*device, memory, nullptr);
        memory = VK_NULL_HANDLE;
        Logger::getInstance().debug("Memory freed successfully.");
    }
    size = 0;
    memory_property_flags = 0;
}

Memory::~Memory() {
    destroy();
}