#pragma once
#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>
#include "ev-device.h"
#include "ev-commandbuffer.h"
#include "ev-swapchain.h"

using namespace std;

namespace ev {

class Queue {

private:

    std::shared_ptr<Device> device;

    VkQueue queue = VK_NULL_HANDLE;

    uint32_t queue_family_index = UINT32_MAX;

    uint32_t queue_index = UINT32_MAX;

    // Disable copy constructor and assignment operator

public:
    explicit Queue(
        std::shared_ptr<Device> _device,
        uint32_t _queue_family_index,
        uint32_t _queue_index = 0
    );

    Queue(const Queue&) = delete;

    Queue& operator=(const Queue&) = delete;

    VkResult submit( shared_ptr<CommandBuffer> buffer);

    VkResult waitIdle(uint64_t timeout = UINT64_MAX);

    VkResult present(std::shared_ptr<Swapchain> swapchain, uint32_t image_index);

    uint32_t get_queue_index() const {
        return queue_index;
    }

    uint32_t get_queue_family_index() const {
        return queue_family_index;
    }

    operator VkQueue() const {
        return queue;
    }

    ~Queue();
};

}