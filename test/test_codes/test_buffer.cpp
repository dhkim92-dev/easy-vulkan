#include <gtest/gtest.h>
#include <easy-vulkan.h>
#include <memory>

using namespace std;

class BufferTest : public ::testing::Test {
    protected:
    std::shared_ptr<ev::Instance> instance;
    std::shared_ptr<ev::PhysicalDevice> physical_device;
    std::shared_ptr<ev::Device> device;

    void SetUp() override {
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

        instance = make_shared<ev::Instance>(required_instance_extensions, required_layers, false);
        ASSERT_TRUE(instance->is_valid());

        // Get physical devices
        auto physical_devices = ev::utility::list_physical_devices(instance->get_instance());
        ASSERT_FALSE(physical_devices.empty());

        // Create physical device
        physical_device = make_shared<ev::PhysicalDevice>(instance, physical_devices[0]);

        vector<const char*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        
        // Create device
        device = make_shared<ev::Device>(instance, physical_device, device_extensions, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
    }
};


TEST_F(BufferTest, CreateBuffer) {
    ev::Buffer buffer(device, 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    ASSERT_TRUE(buffer.get_size() == 1024);
    ASSERT_TRUE( (buffer.get_usage_flags() | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

