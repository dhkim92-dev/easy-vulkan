#include <gtest/gtest.h>
#include "easy-vulkan.h"

class InstanceTest : public ::testing::Test {
protected:
    static ev::Instance* instance;
    static void SetUpTestSuite() {
        std::vector<const char*> required_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            #ifdef __APPLE__
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            #endif
        };
        
        std::vector<const char*> required_layers = {"VK_LAYER_KHRONOS_validation"};
        
        instance = new ev::Instance(required_extensions, required_layers, true);
    }
    static void TearDownTestSuite() {
        delete instance;
        instance = nullptr;
    }
};

ev::Instance* InstanceTest::instance = nullptr;

TEST_F(InstanceTest, constructor_test) {
    std::vector<const char*> required_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #ifdef __APPLE__
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
    };

    std::vector<const char*> required_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    ev::Instance instance(required_extensions, required_layers, true);

    EXPECT_TRUE(instance.is_valid()) << "Instance should be valid after creation.";
    EXPECT_FALSE(instance.get_enabled_extensions().empty()) << "Instance extensions should not be empty.";
    EXPECT_FALSE(instance.get_enabled_layers().empty()) << "Instance validation layers should not be empty.";
}

TEST_F(InstanceTest, is_support_extension_test) {
    ev::Instance instance({
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #ifdef __APPLE__
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
    }, {"VK_LAYER_KHRONOS_validation"}, true);

    EXPECT_TRUE(instance.is_support_extension(VK_KHR_SURFACE_EXTENSION_NAME)) << "Should support VK_KHR_surface extension.";
    EXPECT_FALSE(instance.is_support_extension("VK_NON_EXISTENT_EXTENSION")) << "Should not support non-existent extension.";
}

TEST_F(InstanceTest, is_support_layer_test) {
    ev::Instance instance({
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #ifdef __APPLE__
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
    }, {"VK_LAYER_KHRONOS_validation"}, true);

    EXPECT_TRUE(instance.is_support_layer("VK_LAYER_KHRONOS_validation")) << "Should support VK_LAYER_KHRONOS_validation layer.";
    EXPECT_FALSE(instance.is_support_layer("VK_NON_EXISTENT_LAYER")) << "Should not support non-existent layer.";
}

TEST_F(InstanceTest, debug_messenger_creation_test) {
    ev::Instance instance({
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #ifdef __APPLE__
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
    }, {"VK_LAYER_KHRONOS_validation"}, true);

    EXPECT_TRUE(instance.is_valid()) << "Instance should be valid after creation.";
    
    // Check if debug messenger is created
    EXPECT_NE(instance.get_instance(), VK_NULL_HANDLE) << "Instance should have a valid handle.";
}

