#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "ev-instance.h"

using namespace std;

namespace ev {

class PhysicalDevice {
 
private: 

    std::shared_ptr<Instance> instance;

    VkPhysicalDevice handle = VK_NULL_HANDLE;

    VkPhysicalDeviceFeatures features;

    VkPhysicalDeviceProperties properties;

    VkPhysicalDeviceMemoryProperties memory_properties;
    
    vector<VkExtensionProperties> extensions;

public: 

    explicit PhysicalDevice(std::shared_ptr<Instance> instance, VkPhysicalDevice device);

    PhysicalDevice& operator=(const PhysicalDevice&) = delete;

    PhysicalDevice(const PhysicalDevice&) = delete;

    operator VkPhysicalDevice() const {
        return handle;
    }

    VkPhysicalDeviceMemoryProperties get_memory_properties() const {
        return memory_properties;
    }

    const VkPhysicalDeviceProperties& get_properties() const {
        return properties;
    }

    const VkPhysicalDeviceFeatures& get_features() const {
        return features;
    }

    const std::shared_ptr<Instance> get_instance() const {
        return instance;
    }

    const vector<VkExtensionProperties>& get_extensions() const {
        return extensions;
    }

    const VkFormatProperties get_format_properties(VkFormat format) const;

    operator VkPhysicalDeviceFeatures() const {
        return features;
    }

    operator VkPhysicalDeviceProperties() const {
        return properties;
    }

    ~PhysicalDevice();
};

}