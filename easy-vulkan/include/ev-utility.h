#pragma once

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <vulkan/vulkan.h>

using namespace std;

namespace ev {

    namespace utility {

        bool file_exists(const char* filename);

        bool is_vulkan_available();

        vector<VkPhysicalDevice> list_physical_devices(VkInstance instance);
        
        vector<VkExtensionProperties> list_instance_extensions();

        vector<VkLayerProperties> list_instance_layers();

        vector<VkExtensionProperties> list_device_extensions(VkPhysicalDevice device);

        // vector<VkLayerProperties> list_device_layers(VkPhysicalDevice device);

        VkPhysicalDeviceFeatures list_device_features(VkPhysicalDevice device);

        VkPhysicalDeviceProperties list_device_properties(VkPhysicalDevice device);

        void read_spirv_shader_file(const char* filename, vector<uint32_t>& code);  
    }
}