#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include "ev-instance.h"
#include "ev-pdevice.h"
#include "ev-macro.h"
#include "ev-logger.h"
#include "ev-utility.h"

namespace ev {

class Device {

    private:

    std::shared_ptr<Instance> instance;

    std::shared_ptr<PhysicalDevice> pdevice;

    VkDevice device = VK_NULL_HANDLE;

    std::vector<const char*> enabled_extensions;

    std::vector<VkQueueFamilyProperties> queue_family_properties;

    bool use_swapchain = true;

    struct QueueFamilyIndices {

        uint32_t graphics = UINT32_MAX;

        uint32_t transfer = UINT32_MAX;

        uint32_t compute = UINT32_MAX;
    } queue_family_indices;

    void check_required_extensions(std::vector<const char*>& required_extensions);

    void setup_queue_family_properties();

    void setup_queue_family_indices(VkQueueFlags flags);

    uint32_t get_queue_family_index(VkQueueFlags flags) const;


    public:

    explicit Device(
        std::shared_ptr<Instance> instance, 
        std::shared_ptr<PhysicalDevice> pdeivce,
        vector<const char*> required_extensions,
        VkQueueFlags queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
        bool use_swapchain = true
    );

    Device& operator=(const Device&) = delete;

    Device(const Device&) = delete;

    ~Device();

    void destroy();

    uint32_t get_queue_index(VkQueueFlags flags) const;

    uint32_t get_memory_type_index(
        uint32_t type_bits,
        VkMemoryPropertyFlags memory_property_flags,
        VkBool32 *found
    ) const;

    VkFormat get_supported_depth_format(
        bool check_sampling_support = false
    ) const;

    VkResult wait_idle(uint64_t timeout = UINT64_MAX) const;

    operator VkDevice() const {
        return device;
    }

    const std::shared_ptr<Instance> get_instance() const {
        return instance;
    }

    const std::shared_ptr<PhysicalDevice> get_physical_device() const {
        return pdevice;
    }

    const std::vector<const char*> get_enabled_extensions() const {
        return enabled_extensions;
    }

    const std::vector<VkQueueFamilyProperties>& get_queue_family_properties() const {
        return queue_family_properties;
    }

    const VkPhysicalDeviceProperties get_properties() const {
        return pdevice->get_properties();
    }

    const VkPhysicalDeviceFeatures get_features() const {
        return pdevice->get_features();
    }

    const bool is_swapchain_enabled() const {
        return use_swapchain;
    }
};

}
