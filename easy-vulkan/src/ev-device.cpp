#include "ev-device.h"
#include <string>
using namespace ev;
using namespace ev::logger;

Device::Device(
    std::shared_ptr<Instance> _instance,
    std::shared_ptr<PhysicalDevice> _pdevice,
    vector<const char*> required_extensions,
    VkQueueFlags queue_flags,
    bool use_swapchain
) : instance(std::move(_instance)), pdevice(std::move(_pdevice)), use_swapchain(use_swapchain) {
    Logger::getInstance().info("[ev::Device] Creating Vulkan device...");

    if ( use_swapchain ) {
        enabled_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

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
    CHECK_RESULT(vkCreateDevice(*pdevice, &device_ci, nullptr, &device));
    Logger::getInstance().info("[ev::Device] Vulkan device created successfully.");
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

uint32_t Device::get_memory_type_index(
    uint32_t type_bits,
    VkMemoryPropertyFlags memory_property_flags,
    VkBool32 *found
) const {

    auto memory_properties = pdevice->get_memory_properties();

    for ( uint32_t i = 0 ; i < memory_properties.memoryTypeCount ; ++i ) {
        if ( (type_bits & 1) == 1 ) {
            if ( (memory_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags ) {
                if ( found ) {
                    *found = VK_TRUE;
                }
                Logger::getInstance().debug("Memory type index found: " + std::to_string(static_cast<int>(i)));
                return i;
            }
        }
        type_bits >>= 1;
    }

    if (found) {
        *found = VK_FALSE;
        return 0;
    }

    Logger::getInstance().error("No suitable memory type index found for the requested size and property flags.");
    exit(EXIT_FAILURE);
}

VkResult Device::wait_idle(uint64_t timeout) const {
    return vkDeviceWaitIdle(device);
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

VkFormat Device::get_supported_depth_format(bool check_sampling_support) const {
    VkFormat depth_formats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D16_UNORM_S8_UINT
    };

    for (const auto& format : depth_formats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(*pdevice, format, &props);
        
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            if (!check_sampling_support || (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
                Logger::getInstance().debug("Supported depth format found: " + std::to_string(format));
                return format;
            }
        }
    }

    Logger::getInstance().error("No suitable depth format found.");
    exit(EXIT_FAILURE);
}

void Device::destroy() {
    Logger::getInstance().info("[ev::Device] Destroying Vulkan device.");
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }
    Logger::getInstance().info("[ev::Device] Vulkan device destroyed.");
}

Device::~Device() {
    destroy();
}
