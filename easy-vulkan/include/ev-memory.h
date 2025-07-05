#pragma once

#include <memory>
#include <vector>
#include "ev-device.h"
#include "ev-instance.h"
#include "ev-utility.h"
#include "ev-logger.h"

using namespace std;

namespace ev {

class Memory {

private:

    std::shared_ptr<Device> device;

    VkDeviceMemory memory = VK_NULL_HANDLE;

    VkDeviceSize size = 0;

    VkMemoryPropertyFlags memory_property_flags = 0;

public:

    explicit Memory(
        std::shared_ptr<Device> device,
        VkDeviceSize size,
        VkMemoryPropertyFlags memory_property_flags,
        VkMemoryRequirements memory_requirements,
        VkMemoryAllocateFlagsInfoKHR* alloc_flags_info = nullptr
    );

    VkResult allocate();

    ~Memory();

    Memory(const Memory&) = delete;

    Memory& operator=(const Memory&) = delete;

    void destroy();

    VkDeviceSize get_size() const {
        return size;
    }

    VkMemoryPropertyFlags get_memory_property_flags() const {
        return memory_property_flags;
    }

    operator VkDeviceMemory() const {
        return memory;
    }
};


}