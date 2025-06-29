#pragma once

#include <memory>
#include <vector>
#include "ev-device.h"


namespace ev {

class Semaphore {

private:

    std::shared_ptr<ev::Device> device;

    VkSemaphore semaphore = VK_NULL_HANDLE;

    public :

    explicit Semaphore(std::shared_ptr<ev::Device> device, 
        VkSemaphoreCreateFlags flags = 0,
        void* next = nullptr);

    Semaphore(const Semaphore&) = delete;

    Semaphore& operator=(const Semaphore&) = delete;

    void destroy();

    ~Semaphore();

    operator VkSemaphore() const {
        return semaphore;
    }
};

class Fence {

private:

    std::shared_ptr<ev::Device> device;

    VkFence fence = VK_NULL_HANDLE;

public :

    explicit Fence(std::shared_ptr<ev::Device> device, VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT, void* next = nullptr);

    Fence(const Fence&) = delete;

    Fence& operator=(const Fence&) = delete;

    VkResult wait(uint64_t timeout = UINT64_MAX);

    VkResult reset();

    void destroy();

    ~Fence();

    operator VkFence() const {
        return fence;
    }   
};

class Event {

private:

    std::shared_ptr<ev::Device> device;

    VkEvent event = VK_NULL_HANDLE;

public:

    explicit Event(std::shared_ptr<ev::Device> device, VkEventCreateFlags flags = 0, void* next = nullptr);

    Event(const Event&) = delete;

    Event& operator=(const Event&) = delete;

    void destroy();

    ~Event();

    operator VkEvent() const {
        return event;
    }
};

}