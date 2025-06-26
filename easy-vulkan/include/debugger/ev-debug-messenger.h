#pragma once
#include <vulkan/vulkan.h>

namespace ev::debugger {

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        fprintf(stdout, "[Vulkan Debug] Message Type: %s\n", 
            (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) ? "General" :
            (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) ? "Validation" :
            (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) ? "Performance" : "Unknown");
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            fprintf(stderr, "[Vulkan Debug] %s\n", pCallbackData->pMessage);
        } else {
            fprintf(stdout, "[Vulkan Debug] %s\n", pCallbackData->pMessage);
        }
        return VK_FALSE;
    }

}