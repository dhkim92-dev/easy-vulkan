#include "ev-sync.h"
#include "ev-device.h"
#include "ev-logger.h"

using namespace ev;

Semaphore::Semaphore(std::shared_ptr<ev::Device> device,
    VkSemaphoreCreateFlags flags,
void* next): device(device) {
    if (!this->device) {
        throw std::runtime_error("Device is null");
    }

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = next;
    semaphore_info.flags = flags;
    VkResult result = vkCreateSemaphore(*device, &semaphore_info, nullptr, &semaphore);
    if (result != VK_SUCCESS) {
        logger::Logger::getInstance().error("Failed to create semaphore: " + std::to_string(result));
        exit(EXIT_FAILURE);
    }
    this->device = device;
    logger::Logger::getInstance().debug("Semaphore created successfully");
}

void Semaphore::destroy() {
    if (semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(*device, semaphore, nullptr);
        semaphore = VK_NULL_HANDLE;
        logger::Logger::getInstance().debug("Semaphore destroyed successfully");
    }
}

Semaphore::~Semaphore() {
    destroy();
}