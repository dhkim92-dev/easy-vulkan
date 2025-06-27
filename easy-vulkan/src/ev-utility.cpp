#include "ev-utility.h"
#include <cassert>

using namespace ev::utility;

bool ev::utility::file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

bool ev::utility::is_vulkan_available() {
    uint32_t version = 0;
    VkResult result = vkEnumerateInstanceVersion(&version);
    if (result == VK_SUCCESS) {
        return true;
    } else {
        fprintf(stderr, "Vulkan is not available: %d\n", result);
        return false;
    }
}

vector<VkPhysicalDevice> ev::utility::list_physical_devices(VkInstance instance) {
    uint32_t device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    vector<VkPhysicalDevice> physical_devices;

    
    if (result != VK_SUCCESS || device_count == 0) {
        fprintf(stderr, "No physical devices found: %d\n", result);
        return physical_devices;
    }

    physical_devices.resize(device_count);
    result = vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to enumerate physical devices: %d\n", result);
        physical_devices.clear();
    }
    
    return physical_devices;
}

vector<VkExtensionProperties> ev::utility::list_instance_extensions() {
    uint32_t count = 0;
    vector<VkExtensionProperties> extensions;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if ( count == 0 ) {
        fprintf(stderr, "No instance extensions available.\n");
        return extensions;
    }
    extensions.resize(count);
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to enumerate instance extensions: %d\n", result);
        extensions.clear();
    } 
    return extensions;
}

vector<VkLayerProperties> ev::utility::list_instance_layers() {
    uint32_t count = 0;
    vector<VkLayerProperties> layers;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    if (count == 0) {
        fprintf(stderr, "No instance layers available.\n");
        return layers;
    }
    layers.resize(count);
    VkResult result = vkEnumerateInstanceLayerProperties(&count, layers.data());

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to enumerate instance layers: %d\n", result);
        layers.clear();
    }
    return layers;
}

vector<VkExtensionProperties> ev::utility::list_device_extensions(VkPhysicalDevice device) {
    uint32_t count = 0;
    vector<VkExtensionProperties> extensions;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    if (count == 0) {
        fprintf(stderr, "No device extensions available.\n");
        return extensions;
    }
    extensions.resize(count);
    VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to enumerate device extensions: %d\n", result);
        extensions.clear();
    }
    return extensions;
}

// Deprecated, 최신 Vulkan API에서는 디바이스 레이어를 사용하지 않습니다ㅈ
// vector<VkLayerProperties> ev::utility::list_device_layers(VkPhysicalDevice device) {
//     uint32_t count = 0;
//     vector<VkLayerProperties> layers;
//     vkEnumerateDeviceLayerProperties(device, &count, nullptr);
//     if (count == 0) {
//         fprintf(stderr, "No device layers available.\n");
//         return layers;
//     }
//     layers.resize(count);
//     VkResult result = vkEnumerateDeviceLayerProperties(device, &count, layers.data());

//     if (result != VK_SUCCESS) {
//         fprintf(stderr, "Failed to enumerate device layers: %d\n", result);
//         layers.clear();
//     }
//     return layers;
// }

VkPhysicalDeviceFeatures ev::utility::list_device_features(VkPhysicalDevice device) {
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    return features;
}

VkPhysicalDeviceProperties ev::utility::list_device_properties(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    return properties;
}

void ev::utility::read_spirv_shader_file(const char* filename, vector<uint32_t>& code) {
    if (!file_exists(filename)) {
        fprintf(stderr, "Shader file does not exist: %s\n", filename);
        return;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filename);
        return;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    code.resize(size / sizeof(uint32_t));
    fread(code.data(), sizeof(uint32_t), code.size(), file);
    fclose(file);
}

