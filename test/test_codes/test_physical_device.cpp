#include <gtest/gtest.h>
#include <memory>
#include "easy-vulkan.h"

using namespace std;

#define DEBUG_MODE false

class PhysicalDeviceTest : public ::testing::Test {
    protected:
    std::shared_ptr<ev::Instance> instance;

    void SetUp() override {
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

        instance = std::make_shared<ev::Instance>(required_extensions, required_layers, true);
    }
};


TEST_F(PhysicalDeviceTest, properties_test) {
    ASSERT_FALSE(!instance->is_valid());

    std::vector<VkPhysicalDevice> devices = ev::utility::list_physical_devices(
        instance->get_instance()
    );
    ASSERT_FALSE(devices.empty()) << "Physical devices should not be empty.";

    ev::PhysicalDevice pdevice(instance, devices[0]);
    VkPhysicalDeviceProperties properties = pdevice;
    EXPECT_FALSE(properties.deviceName[0] == '\0') << "Device name should not be empty.";
}

TEST_F(PhysicalDeviceTest, physical_device_feature_test) {
    ASSERT_FALSE(!instance->is_valid());
    std::vector<VkPhysicalDevice> devices = ev::utility::list_physical_devices(
        instance->get_instance()
    );
    ASSERT_FALSE(devices.empty()) << "Physical devices should not be empty.";

    ev::PhysicalDevice pdevice(instance, devices[0]);
    VkPhysicalDeviceFeatures features = pdevice;
    #ifdef __APPLE__
    EXPECT_FALSE(features.geometryShader) << "Geometry shader should not be supported on Apple platforms.";
    #else
    EXPECT_TRUE(features.geometryShader) << "Geometry shader should be supported.";
    #endif
}

TEST_F(PhysicalDeviceTest, constructor_test) {
    shared_ptr<ev::Instance> instance = std::make_shared<ev::Instance>(
        std::vector<const char*>{
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    #ifdef __APPLE__
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    #endif
        },
        std::vector<const char*>{"VK_LAYER_KHRONOS_validation"},
        DEBUG_MODE
    );

    std::vector<VkPhysicalDevice> devices = ev::utility::list_physical_devices(
        instance->get_instance()
    );
    ASSERT_FALSE(devices.empty()) << "Physical devices should not be empty.";

    for (const auto& device : devices) {
        ev::PhysicalDevice pdevice(instance, device);
    }
}