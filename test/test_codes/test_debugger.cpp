
#include <gtest/gtest.h>
#include <easy-vulkan.h>

using namespace std;
using namespace ev;


TEST(DebuggerTest, setup_debug_mode_on) {
    vector<const char*> required_instance_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #ifdef __APPLE__
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
    };

    vector<const char*> required_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    ev_log_set_log_level(ev::logger::LogLevel::DEBUG);

    ev::Instance instance(required_instance_extensions, required_layers, true);
    ASSERT_TRUE(instance.is_valid());
}
