#pragma once
#include <vulkan/vulkan.h>

namespace ev::debugger {

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            fprintf(stderr, "[Vulkan Debug] %s\n", pCallbackData->pMessage);
        } else {
            fprintf(stdout, "[Vulkan Debug] %s\n", pCallbackData->pMessage);
        }
        return VK_FALSE;
    }

}