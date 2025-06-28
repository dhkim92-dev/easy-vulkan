#include "ev-sync.h"

using namespace ev;


Event::Event(std::shared_ptr<Device> device, VkEventCreateFlags flags, void* next)
    : device(std::move(device)) {
    if (!this->device) {
        throw std::runtime_error("Device is null");
    }

    VkEventCreateInfo event_info = {};
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    event_info.pNext = next;
    event_info.flags = flags;

    VkResult result = vkCreateEvent(*this->device, &event_info, nullptr, &event);
    if (result != VK_SUCCESS) {
        logger::Logger::getInstance().error("Failed to create event: " + std::to_string(result));
        exit(EXIT_FAILURE);
    }
    logger::Logger::getInstance().debug("Event created successfully");
}

void Event::destroy() {
    if (event != VK_NULL_HANDLE) {
        vkDestroyEvent(*device, event, nullptr);
        event = VK_NULL_HANDLE;
        logger::Logger::getInstance().debug("Event destroyed successfully");
    }
}

Event::~Event() {
    destroy();
}
