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

TEST_F(BufferTest, BindMemory) {
    shared_ptr<ev::Buffer> buffer = make_shared<ev::Buffer>(device, 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    VkMemoryRequirements memory_requirements = buffer->get_memory_requirements();
    shared_ptr<ev::Memory> memory = make_shared<ev::Memory>(device, 1024, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memory_requirements, nullptr);
    VkResult result = buffer->bind_memory(memory, 0);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_TRUE(buffer->get_size() == 1024);
}

TEST_F(BufferTest, MapUnmapBuffer) {
    shared_ptr<ev::Buffer> buffer = make_shared<ev::Buffer>(device, 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    VkMemoryRequirements memory_requirements = buffer->get_memory_requirements();
    shared_ptr<ev::Memory> memory = make_shared<ev::Memory>(device,
        1024, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        memory_requirements, 
        nullptr
    );
    VkResult result = buffer->bind_memory(memory, 0);
    ASSERT_EQ(result, VK_SUCCESS);

    char sample_data[] = "Hello, Vulkan!";
    void *ptr = sample_data;
    result = buffer->map(1024);
    ASSERT_EQ(result, VK_SUCCESS);
    buffer->write(ptr, sizeof(sample_data));
    ASSERT_TRUE(buffer->get_mapped_ptr() != nullptr);
    char *read_data = new char[sizeof(sample_data)];
    result = buffer->read(read_data, sizeof(sample_data));
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_STREQ(read_data, sample_data);
    delete [] read_data;
    result = buffer->unmap();
    ASSERT_EQ(result, VK_SUCCESS);
}   