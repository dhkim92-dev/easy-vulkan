#include "ev-sync.h"

using namespace std;

namespace ev {

Fence::Fence(shared_ptr<Device> device, VkFenceCreateFlags flags, void* next) 
    : device(std::move(device)) {
    ev::logger::Logger::getInstance().info("[ev::Fence] Creating fence with flags: " + std::to_string(flags));
    if (!this->device) {
        ev::logger::Logger::getInstance().error("[ev::Fence] Device is null. Cannot create fence.");
        exit(EXIT_FAILURE);
    }

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = flags;
    fence_info.pNext = next;

    VkResult result = vkCreateFence(*this->device, &fence_info, nullptr, &fence);
    if (result != VK_SUCCESS) {
        logger::Logger::getInstance().error("[ev::Fence] Failed to create fence: " + std::to_string(result));
        exit(EXIT_FAILURE);
    }
    logger::Logger::getInstance().info("[ev::Fence] Fence created successfully");
}

VkResult Fence::wait(uint64_t timeout) {
    if (fence == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("[ev::Fence] Fence is not initialized");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkResult result = vkWaitForFences(*device, 1, &fence, VK_TRUE, timeout);
    if (result != VK_SUCCESS) {
        logger::Logger::getInstance().error("[ev::Fence] Failed to wait for fence: " + std::to_string(result));
    } else {
        logger::Logger::getInstance().debug("[ev::Fence] Fence waited successfully");
    }
    return result;
}

VkResult Fence::reset() {
    if (fence == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Fence is not initialized");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkResult result = vkResetFences(*device, 1, &fence);
    if (result != VK_SUCCESS) {
        logger::Logger::getInstance().error("Failed to reset fence: " + std::to_string(result));
    } else {
        logger::Logger::getInstance().debug("Fence reset successfully");
    }
    return result;
}

void Fence::destroy() {
    ev::logger::Logger::getInstance().info("[ev::Fence::destroy] Destroying fence.");
    if (fence != VK_NULL_HANDLE) {
        vkDestroyFence(*device, fence, nullptr);
        logger::Logger::getInstance().debug("Fence destroyed successfully");
    }
    ev::logger::Logger::getInstance().debug("[ev::Fence::destroy] Fence already destroyed or not initialized.");
}

Fence::~Fence() {
    destroy();
}

};
