#pragma once

#include <vulkan/vulkan.h>
#include <string>

#define CHECK_RESULT(f) \
{ \
    VkResult result = (f); \
    if (result != VK_SUCCESS) { \
        fprintf(stderr, "[ERROR][%s:%d]: Vulkan error (%s)\n", __FILE__, __LINE__, ev::macro::error_string(result).c_str()); \
        exit(EXIT_FAILURE); \
    } \
}

namespace ev {
namespace macro {

std::string error_string(VkResult errorCode);

}
}