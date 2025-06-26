#include <gtest/gtest.h>
#include "easy-vulkan.h"

class ImageTest : public ::testing::Test {
protected:
    std::shared_ptr<ev::Instance> instance;
    std::shared_ptr<ev::PhysicalDevice> physical_device;
    std::shared_ptr<ev::Device> device;

    void SetUp() override {
        // ev::logger::Logger::getInstance().set_log_level(INFO);
        instance = std::make_shared<ev::Instance>(std::vector<const char*>(), std::vector<const char*>(), false);
        physical_device = std::make_shared<ev::PhysicalDevice>(instance, ev::utility::list_physical_devices(*instance)[0]);
        device = std::make_shared<ev::Device>(instance, physical_device, std::vector<const char*>());
    }
};

TEST_F(ImageTest, CreateImage) {
    ev::Image image(device, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
    EXPECT_NE(VkImage(image), VK_NULL_HANDLE) << "Image should be created successfully.";
}

TEST_F(ImageTest, BindMemory) {
    std::shared_ptr<ev::Image> image = make_shared<ev::Image>(device, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
    VkMemoryRequirements req = image->get_memory_requirements();
    std::shared_ptr<ev::Memory> memory = make_shared<ev::Memory>(
        device, 
        800 * 600 * 4,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        req
    );

    VkResult result = image->bind_memory(memory);
    EXPECT_EQ(result, VK_SUCCESS);
}