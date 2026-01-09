#include "ev-sync.h"

using namespace ev;


Event::Event(std::shared_ptr<Device> device, VkEventCreateFlags flags, void* next)
    : device(std::move(device)) {
    ev_log_info("[ev::Event] Creating event with flags: %u", flags);
    if (!this->device) {
        ev_log_error("[ev::Event] Device is null. Cannot create event.");
        exit(EXIT_FAILURE);
    }

    VkEventCreateInfo event_info = {};
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    event_info.pNext = next;
    event_info.flags = flags;

    VkResult result = vkCreateEvent(*this->device, &event_info, nullptr, &event);
    if (result != VK_SUCCESS) {
        ev_log_error("Failed to create event: %d", static_cast<int>(result));
        exit(EXIT_FAILURE);
    }
    ev_log_info("[ev::Event] Event created successfully");
}

void Event::destroy() {
    ev_log_info("[ev::Event::destroy] Destroying event.");
    if (event != VK_NULL_HANDLE) {
        vkDestroyEvent(*device, event, nullptr);
        event = VK_NULL_HANDLE;
    }
    ev_log_info("[ev::Event] Event destroyed successfully");
}

Event::~Event() {
    destroy();
}
