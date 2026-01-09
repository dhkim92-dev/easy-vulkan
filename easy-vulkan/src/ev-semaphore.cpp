#include "ev-sync.h"
#include "ev-device.h"
#include "ev-logger.h"

using namespace ev;

Semaphore::Semaphore(std::shared_ptr<ev::Device> device,
    VkSemaphoreCreateFlags flags,
void* next): device(device) {
    ev_log_info("[ev::Semaphore] Creating semaphore with flags: %u", static_cast<unsigned int>(flags));
    if (!this->device) {
        ev_log_error("Device is null. Cannot create semaphore.");
        exit(EXIT_FAILURE);
    }

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = next;
    semaphore_info.flags = flags;
    VkResult result = vkCreateSemaphore(*device, &semaphore_info, nullptr, &semaphore);
    if (result != VK_SUCCESS) {
        ev_log_error("Failed to create semaphore: %d", result);
        exit(EXIT_FAILURE);
    }
    this->device = device;
    ev_log_info("Semaphore created successfully");
}

void Semaphore::destroy() {
    ev_log_info("[ev::Semaphore::destroy] Destroying semaphore.");
    if (semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(*device, semaphore, nullptr);
        semaphore = VK_NULL_HANDLE;
        ev_log_info("Semaphore destroyed successfully");
    }
    ev_log_debug("[ev::Semaphore::destroy] Semaphore already destroyed or not initialized.");
}

Semaphore::~Semaphore() {
    destroy();
}