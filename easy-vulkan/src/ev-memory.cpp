
#include "ev-memory.h"

using namespace ev;
using namespace ev::logger;

Memory::Memory(
    std::shared_ptr<Device> device,
    VkDeviceSize size,
    VkMemoryPropertyFlags memory_property_flags,
    VkMemoryRequirements memory_requirements,
    VkMemoryAllocateFlagsInfoKHR* alloc_flags_info
) : device(device), size(size), memory_property_flags(memory_property_flags) {
    Logger::getInstance().debug("Creating Memory with size: " + std::to_string(size));
    if (!device) {
        Logger::getInstance().error("Invalid device provided for Memory creation");
        exit(EXIT_FAILURE);
    }
    VkMemoryAllocateInfo mem_ai = {};
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.allocationSize = size;
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
    Logger::getInstance().debug("Destroying Memory...");
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(*device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
    size = 0;
    memory_property_flags = 0;
    Logger::getInstance().debug("Memory destroyed successfully.");
}

Memory::~Memory() {
    destroy();
}