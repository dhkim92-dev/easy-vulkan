#pragma once

#include <vulkan/vulkan.h>

namespace ev {
namespace initializer {

    inline VkApplicationInfo create_application_info(const char* applicationName, 
        uint32_t applicationVersion,
        const char* engineName, 
        uint32_t engineVersion, 
        uint32_t apiVersion) {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = applicationName;
        appInfo.applicationVersion = applicationVersion;
        appInfo.pEngineName = engineName;
        appInfo.engineVersion = engineVersion;
        appInfo.apiVersion = apiVersion;
        return appInfo;
    }

    inline VkInstanceCreateInfo instance_create_info(
        const VkApplicationInfo* pApplicationInfo = nullptr, 
        uint32_t enabledLayerCount = 0, 
        const char* const* ppEnabledLayerNames = nullptr, 
        uint32_t enabledExtensionCount = 0, 
        const char* const* ppEnabledExtensionNames = nullptr
    ) {
        VkInstanceCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pNext = nullptr;
        ci.flags = 0;
        ci.pApplicationInfo = pApplicationInfo;
        ci.enabledLayerCount = enabledLayerCount;
        ci.ppEnabledLayerNames = ppEnabledLayerNames;
        ci.enabledExtensionCount = enabledExtensionCount;
        ci.ppEnabledExtensionNames = ppEnabledExtensionNames;
        return ci;
    }

    inline VkDeviceCreateInfo device_create_info() {
        VkDeviceCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        return ci;
    }

}}