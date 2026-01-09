#include <gtest/gtest.h>
#include "easy-vulkan.h"
#include <memory>

class DeviceTest : public ::testing::Test {
protected:
    shared_ptr<ev::Instance> instance;
    shared_ptr<ev::PhysicalDevice> pdevice;
    shared_ptr<ev::Device> device;

    void SetUp() override {
    ev_log_set_log_level(ev::logger::LogLevel::ERROR);
        vector<const char*> required_instance_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            #ifdef __APPLE__
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            #endif
        };
        
        vector<const char*> required_layers = {"VK_LAYER_KHRONOS_validation"};
        if ( instance == nullptr ) {
            instance = make_shared<ev::Instance>(required_instance_extensions, required_layers, true);
        }

        ASSERT_TRUE(instance->is_valid()) << "Instance should be valid.";
        vector<VkPhysicalDevice> devices = ev::utility::list_physical_devices(instance->get_instance());
        ASSERT_FALSE(devices.empty()) << "Physical devices should not be empty.";
        pdevice = make_shared<ev::PhysicalDevice>(instance, devices[0]);
        vector<const char*> required_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        device = make_shared<ev::Device>(instance, pdevice, required_extensions, 
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
    }
};

TEST_F(DeviceTest, constructor_test) {
    EXPECT_TRUE(VkDevice(*device) != VK_NULL_HANDLE) << "Device instance should be valid.";
}

TEST_F(DeviceTest, get_physical_device_test) {
    ASSERT_EQ(device->get_physical_device(), pdevice) << "Physical device should match the created physical device.";
}

TEST_F(DeviceTest, get_instance_test) {
    ASSERT_EQ(device->get_instance(), instance) << "Device instance should match the created instance.";
}

TEST_F(DeviceTest, get_features_test) {
    VkPhysicalDeviceFeatures features = device->get_features();
    ASSERT_TRUE(features.samplerAnisotropy) << "Sampler anisotropy should be supported.";
}

TEST_F(DeviceTest, get_properties_test) {
    VkPhysicalDeviceProperties properties = device->get_properties();
    ASSERT_FALSE(properties.deviceName[0] == '\0') << "Device name should not be empty.";
    ASSERT_GT(properties.apiVersion, 0) << "API version should be greater than 0.";
}

TEST_F(DeviceTest, get_queue_index_test) {
    uint32_t graphics_index = device->get_queue_index(VK_QUEUE_GRAPHICS_BIT);
    ASSERT_NE(graphics_index, UINT32_MAX) << "Graphics queue index should be valid.";
    
    uint32_t compute_index = device->get_queue_index(VK_QUEUE_COMPUTE_BIT);
    ASSERT_NE(compute_index, UINT32_MAX) << "Compute queue index should be valid.";
    
    uint32_t transfer_index = device->get_queue_index(VK_QUEUE_TRANSFER_BIT);
    ASSERT_NE(transfer_index, UINT32_MAX) << "Transfer queue index should be valid.";
}