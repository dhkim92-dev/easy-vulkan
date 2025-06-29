#include <gtest/gtest.h>
#include <easy-vulkan.h>
#include <memory>

using namespace std;
using namespace ev;

class ImageViewTest : public ::testing::Test {

protected:

    shared_ptr<Instance> instance;

    shared_ptr<PhysicalDevice> physical_device;

    shared_ptr<Device> device;

    shared_ptr<Image> image;

    shared_ptr<Memory> memory;

    void SetUp() override {
        ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::ERROR);
        ::testing::GTEST_FLAG(death_test_style) = "threadsafe";
        instance = std::make_shared<ev::Instance>(std::vector<const char*>(), std::vector<const char*>(), false);
        physical_device = std::make_shared<ev::PhysicalDevice>(instance, ev::utility::list_physical_devices(*instance)[0]);
        device = std::make_shared<ev::Device>(instance, physical_device, std::vector<const char*>());
        // Create a simple image for testing
        image = make_shared<Image>(
            device,
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM,
            256, 256, 1,
            1, 1,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0, nullptr, nullptr
        );
        memory = make_shared<Memory>(device, 256*256*4, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image->get_memory_requirements());
        image->bind_memory(memory);
    }
};

TEST_F(ImageViewTest, CreateImageView) {
    VkComponentMapping components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    ImageView image_view(device, image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, components, subresource_range);
    EXPECT_NE(VkImageView(image_view), VK_NULL_HANDLE);
    EXPECT_EQ(image_view.get_view_format(), image->get_format());
    EXPECT_EQ(image_view.get_view_type(), VK_IMAGE_VIEW_TYPE_2D);
}

TEST_F(ImageViewTest, SupportedFormatCompatibility) {
    VkComponentMapping components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    ImageView image_view(device, image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB);
    EXPECT_NE(VkImageView(image_view), VK_NULL_HANDLE);
}   

TEST_F(ImageViewTest, UnsupportedFormatCompatibility) {
    VkComponentMapping components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    ASSERT_DEATH({
        ImageView image_view(device, image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R64G64B64_SFLOAT, components, subresource_range);
    }, ".*");
}   

TEST_F(ImageViewTest, SupportedImageViewType) {
    ImageView image_view(device, image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(image_view.get_view_type(), VK_IMAGE_VIEW_TYPE_2D);
}

TEST_F(ImageViewTest, UnsupportedImageViewType) {
    ASSERT_DEATH({
        ImageView image_view(device, image, VK_IMAGE_VIEW_TYPE_1D, VK_FORMAT_R8G8B8A8_UNORM);
    }, ".*");
}
