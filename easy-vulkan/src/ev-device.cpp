#include "ev-device.h"

using namespace ev;
using namespace ev::logger;

Device::Device(
    std::shared_ptr<Instance> instance,
    std::shared_ptr<PhysicalDevice> pdevice,
    vector<const char*> required_extensions,
    VkQueueFlags queue_flags
) : instance(instance), pdevice(pdevice) {
    Logger::getInstance().info("Creating Vulkan device...");

    check_required_extensions(required_extensions);
    setup_queue_family_properties();
    setup_queue_family_indices(queue_flags);

    const float queue_priority = 1.0f;
    VkDeviceCreateInfo device_ci = initializer::device_create_info();
    device_ci.ppEnabledExtensionNames = enabled_extensions.data();
    device_ci.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
    device_ci.queueCreateInfoCount = 1;
    VkDeviceQueueCreateInfo queue_ci = {};
    queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci.queueFamilyIndex = get_queue_family_index(queue_flags);
    queue_ci.queueCount = 1;
    queue_ci.pQueuePriorities = &queue_priority;
    device_ci.pQueueCreateInfos = &queue_ci;
    device_ci.pNext = nullptr; // No additional structures
    device_ci.enabledLayerCount = 0; // Layers are deprecated in Vulkan 1.2+
    device_ci.ppEnabledLayerNames = nullptr; // No layers enabled
    device_ci.pEnabledFeatures = &pdevice->get_features(); // Use physical device features
    VkResult result = vkCreateDevice(*pdevice, &device_ci, nullptr, &device);
    if (result != VK_SUCCESS) {
        Logger::getInstance().error("Failed to create Vulkan device: " + std::to_string(result));
        exit(EXIT_FAILURE);
    }
    Logger::getInstance().info("Vulkan device created successfully.");
}

uint32_t Device::get_queue_family_index(VkQueueFlags flags) const {

    if ((flags & VK_QUEUE_COMPUTE_BIT) == flags ) {
        for (uint32_t i = 0; i < queue_family_properties.size(); ++i) {
            if ((queue_family_properties[i].queueFlags & flags) && 
                (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
                Logger::getInstance().debug("Queue family index found: " + std::to_string(i));
                return i;
            }
        }
    } 
    
    if ( (flags & VK_QUEUE_TRANSFER_BIT) == flags ) {
        for (uint32_t i = 0; i < queue_family_properties.size(); ++i) {
            if ((queue_family_properties[i].queueFlags & flags) && 
                (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
                (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0) {
                Logger::getInstance().debug("Queue family index found: " + std::to_string(i));
                return i;
            }
        }
    }

    for (uint32_t i = 0; i < queue_family_properties.size(); ++i) {
        if (queue_family_properties[i].queueFlags & flags) {
            Logger::getInstance().debug("Queue family index found: " + std::to_string(i));
            return i;
        }
    }

    Logger::getInstance().error("No suitable queue family found for the requested flags: " + std::to_string(flags));
    exit(EXIT_FAILURE);
}

uint32_t Device::get_queue_index(VkQueueFlags flags) const {
    if (flags & VK_QUEUE_GRAPHICS_BIT) {
        return queue_family_indices.graphics;
    } else if (flags & VK_QUEUE_TRANSFER_BIT) {
        return queue_family_indices.transfer;
    } else if (flags & VK_QUEUE_COMPUTE_BIT) {
        return queue_family_indices.compute;
    } else {
        if ( queue_family_indices.graphics != UINT32_MAX ) {
            return queue_family_indices.graphics;
        }     
    }
    Logger::getInstance().error("No suitable queue index found for the requested flags: " + std::to_string(flags));
    exit(EXIT_FAILURE);
}

void Device::setup_queue_family_indices(VkQueueFlags flags) {
    queue_family_indices.graphics = get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);
    queue_family_indices.transfer = get_queue_family_index(VK_QUEUE_TRANSFER_BIT);
    queue_family_indices.compute = get_queue_family_index(VK_QUEUE_COMPUTE_BIT);

    Logger::getInstance().debug("Queue family indices set: "
        "Graphics: " + std::to_string(queue_family_indices.graphics) +
        ", Transfer: " + std::to_string(queue_family_indices.transfer) +
        ", Compute: " + std::to_string(queue_family_indices.compute));
}

void Device::setup_queue_family_properties() {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*pdevice, &count, nullptr);
    if (count == 0) {
        Logger::getInstance().error("No queue family properties found.");
        exit(EXIT_FAILURE);
    }
    queue_family_properties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(*pdevice, &count, queue_family_properties.data());
}

void Device::check_required_extensions(vector<const char*>& required_extensions) {
    vector<VkExtensionProperties> available_extensions = pdevice->get_extensions();
    #ifdef __APPLE__
    required_extensions.push_back("VK_KHR_portability_subset");
    #endif

    for (const auto& ext_name : required_extensions) {
        bool found = false;
        for (const auto& ext : available_extensions) {
            // Logger::getInstance().info("Checking extension: " + std::string(ext.extensionName));
            if (strcmp(ext.extensionName, ext_name) == 0) {
                found = true;
                Logger::getInstance().debug("Required extension supported: " + std::string(ext_name));
                break;
            }
        }
        if (!found) {
            Logger::getInstance().error("Required device extension not supported: " + std::string(ext_name));
            exit(EXIT_FAILURE);
        }

                enabled_extensions.push_back(ext_name);
    }
}

Device::~Device() {
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
        Logger::getInstance().info("Vulkan device destroyed.");
    }
}