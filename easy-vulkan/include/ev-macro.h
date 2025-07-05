#pragma once

#include <vulkan/vulkan.h>
#include <string>

#define KB 1024
#define MB (1024 * KB)
#define GB (1024 * MB)

#define CHECK_RESULT(f) \
{ \
    VkResult __result = (f); \
    if (__result != VK_SUCCESS) { \
        fprintf(stdout, "[Vulkan ERROR][%s:%d]: Vulkan error (%s)\n", __FILE__, __LINE__, ev::macro::error_string(__result).c_str()); \
        exit(EXIT_FAILURE); \
    } \
}

namespace ev {
namespace macro {

std::string error_string(VkResult errorCode);

}
}