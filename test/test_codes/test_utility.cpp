#include <gtest/gtest.h>
#include <string>
#include "easy-vulkan.h"

#ifndef EASY_VULKAN_ROOT_DIR
    #define EASY_VULKAN_ROOT_DIR "/path/to/easy-vulkan"
#endif

class UtilityTest : public ::testing::Test {
    protected:
    static ev::Instance *instance;

    static void SetUpTestSuite() {

        // Initialize Vulkan instance for all tests
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

ev::Instance* UtilityTest::instance = nullptr;

TEST_F(UtilityTest, file_exists_test) {
        std::string exists_file_name = std::string(EASY_VULKAN_ROOT_DIR) + "/test/test_codes/test_utility.cpp";
    const char *not_exists_file_name = "non_existent_file.txt";
    EXPECT_TRUE(ev::utility::file_exists(exists_file_name.c_str())) << "File should exist.";
    EXPECT_FALSE(ev::utility::file_exists(not_exists_file_name)) << "File should not exist.";
}

TEST_F(UtilityTest, is_vulkan_available_test) {
    EXPECT_TRUE(ev::utility::is_vulkan_available()) << "Vulkan should be available.";
}

TEST_F(UtilityTest, list_instance_extensions_test) {
    std::vector<VkExtensionProperties> extensions = ev::utility::list_instance_extensions();
    EXPECT_FALSE(extensions.empty()) << "Instance extensions should not be empty.";
    
    // for (const auto& ext : extensions) {
    //     std::cout << "Extension: " << ext.extensionName << ", Version: " << ext.specVersion << std::endl;
    // }
}

TEST_F(UtilityTest, list_instance_layers_test) {
    std::vector<VkLayerProperties> layers = ev::utility::list_instance_layers();
    EXPECT_FALSE(layers.empty()) << "Instance layers should not be empty.";
    
    // for (const auto& layer : layers) {
    //     std::cout << "Layer: " << layer.layerName << ", Spec Version: " << layer.specVersion << std::endl;
    // }
}

TEST_F(UtilityTest, list_physical_devices_test) {
    std::vector<VkPhysicalDevice> devices = ev::utility::list_physical_devices(instance->get_instance());
    EXPECT_FALSE(devices.empty()) << "Physical devices should not be empty.";
    
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        EXPECT_NE(properties.deviceName[0], '\0') << "Device name should not be empty.";
    }
}

TEST_F(UtilityTest, list_device_extensions_test) {
    std::vector<VkPhysicalDevice> devices = ev::utility::list_physical_devices(instance->get_instance());
    ASSERT_FALSE(devices.empty()) << "Physical devices should not be empty.";
    
    for (const auto& device : devices) {
        std::vector<VkExtensionProperties> extensions = ev::utility::list_device_extensions(device);
        EXPECT_FALSE(extensions.empty()) << "Device extensions should not be empty.";
        
        // for (const auto& ext : extensions) {
        //     std::cout << "Device Extension: " << ext.extensionName << ", Version: " << ext.specVersion << std::endl;
        // }
    }
}

TEST_F(UtilityTest, list_device_features_test) {
    std::vector<VkPhysicalDevice> devices = ev::utility::list_physical_devices(instance->get_instance());
    ASSERT_FALSE(devices.empty()) << "Physical devices should not be empty.";
    
    for (const auto& device : devices) {
        VkPhysicalDeviceFeatures features = ev::utility::list_device_features(device);
    //     std::cout << "Device Features: " << std::endl;
    //     std::cout << "Geometry Shader: " << features.geometryShader << std::endl;
    //     std::cout << "Tessellation Shader: " << features.tessellationShader << std::endl;
        #ifdef __APPLE__
        EXPECT_FALSE(features.geometryShader) << "Geometry shader should not be supported on Apple platforms.";
        #else
        EXPECT_TRUE(features.geometryShader) << "Geometry shader should be supported.";
        #endif
    }
}

TEST_F(UtilityTest, list_device_properties_test) {
    std::vector<VkPhysicalDevice> devices = ev::utility::list_physical_devices(instance->get_instance());
    ASSERT_FALSE(devices.empty()) << "Physical devices should not be empty.";
    
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties properties = ev::utility::list_device_properties(device);
        EXPECT_NE(properties.deviceName[0], '\0') << "Device name should not be empty.";
    }
}