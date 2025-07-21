#include "ev-sync.h"

using namespace ev;


Event::Event(std::shared_ptr<Device> device, VkEventCreateFlags flags, void* next)
    : device(std::move(device)) {
    ev::logger::Logger::getInstance().info("[ev::Event] Creating event with flags: " + std::to_string(flags));
    if (!this->device) {
        ev::logger::Logger::getInstance().error("[ev::Event] Device is null. Cannot create event.");
        exit(EXIT_FAILURE);
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
    logger::Logger::getInstance().info("[ev::Event] Event created successfully");
}

void Event::destroy() {
    ev::logger::Logger::getInstance().info("[ev::Event::destroy] Destroying event.");
    if (event != VK_NULL_HANDLE) {
        vkDestroyEvent(*device, event, nullptr);
        event = VK_NULL_HANDLE;
    }
    ev::logger::Logger::getInstance().info("[ev::Event] Event destroyed successfully");
}

Event::~Event() {
    destroy();
}
