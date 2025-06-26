
#include <gtest/gtest.h>
#include <easy-vulkan.h>

using namespace std;
using namespace ev;

class CommandPoolTest : public ::testing::Test {
protected:
    shared_ptr<Instance> instance;
    shared_ptr<PhysicalDevice> physical_device;
    shared_ptr<Device> device;

    void SetUp() override {
        ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::DEBUG);
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
        instance = make_shared<ev::Instance>(required_instance_extensions, required_layers, true);
        physical_device = make_shared<ev::PhysicalDevice>(instance, ev::utility::list_physical_devices(instance->get_instance())[0]);
        device = make_shared<ev::Device>(instance, physical_device, vector<const char*>(), VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
    }
};

TEST_F(CommandPoolTest, CreateCommandPool) {
    ev::CommandPool command_pool(device, VK_QUEUE_GRAPHICS_BIT);
    EXPECT_NE(VkCommandPool(command_pool), VK_NULL_HANDLE);
}

TEST_F(CommandPoolTest, AllocateCommandBuffer) {
    ev::CommandPool command_pool(device, VK_QUEUE_GRAPHICS_BIT);
    shared_ptr<ev::CommandBuffer> command_buffer = command_pool.allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    EXPECT_NE(VkCommandBuffer(*command_buffer), VK_NULL_HANDLE);
}

TEST_F(CommandPoolTest, AllocateMultipleCommandBuffers) {
    ev::CommandPool command_pool(device, VK_QUEUE_GRAPHICS_BIT);
    vector<shared_ptr<ev::CommandBuffer>> command_buffers = command_pool.allocate(5, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    EXPECT_EQ(command_buffers.size(), 5);
    for (const auto& cmd_buffer : command_buffers) {
        EXPECT_NE(VkCommandBuffer(*cmd_buffer), VK_NULL_HANDLE);
    }
}