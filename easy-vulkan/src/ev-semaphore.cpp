#include "ev-sync.h"
#include "ev-device.h"
#include "ev-logger.h"

using namespace ev;

Semaphore::Semaphore(std::shared_ptr<ev::Device> device,
    VkSemaphoreCreateFlags flags,
void* next): device(device) {
    ev::logger::Logger::getInstance().info("[ev::Semaphore] Creating semaphore with flags: " + std::to_string(flags));
    if (!this->device) {
        ev::logger::Logger::getInstance().error("Device is null. Cannot create semaphore.");
        exit(EXIT_FAILURE);
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
    logger::Logger::getInstance().info("Semaphore created successfully");
}

void Semaphore::destroy() {
    ev::logger::Logger::getInstance().info("[ev::Semaphore::destroy] Destroying semaphore.");
    if (semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(*device, semaphore, nullptr);
        semaphore = VK_NULL_HANDLE;
        logger::Logger::getInstance().info("Semaphore destroyed successfully");
    }
    ev::logger::Logger::getInstance().debug("[ev::Semaphore::destroy] Semaphore already destroyed or not initialized.");
}

Semaphore::~Semaphore() {
    destroy();
}