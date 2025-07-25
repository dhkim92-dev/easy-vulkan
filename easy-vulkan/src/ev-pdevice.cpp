#include "ev-pdevice.h"
#include "ev-logger.h"

using namespace ev;

PhysicalDevice::PhysicalDevice(std::shared_ptr<Instance> _instance, VkPhysicalDevice device)
    : instance(std::move(_instance)), handle(device) {
    logger::Logger::getInstance().info("Creating PhysicalDevice...");
    if (!instance || !instance->is_valid()) {
        logger::Logger::getInstance().error("Invalid Vulkan instance provided.");
        exit(EXIT_FAILURE);
    }
    if (handle == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Invalid VkPhysicalDevice handle provided.");
        exit(EXIT_FAILURE);
    }
    logger::Logger::getInstance().info("PhysicalDevice created successfully.");

    properties = ev::utility::list_device_properties(handle);
    features = ev::utility::list_device_features(handle);
    extensions = ev::utility::list_device_extensions(handle);
    vkGetPhysicalDeviceMemoryProperties(handle, &memory_properties);
}

const VkFormatProperties PhysicalDevice::get_format_properties(VkFormat format) const {
    VkFormatProperties format_properties;   
    vkGetPhysicalDeviceFormatProperties(handle, format, &format_properties);
    return format_properties;
}

PhysicalDevice::~PhysicalDevice() {
    if (handle != VK_NULL_HANDLE) {
        // Cleanup code if necessary
        handle = VK_NULL_HANDLE;
        logger::Logger::getInstance().info("PhysicalDevice destroyed.");
    }
}