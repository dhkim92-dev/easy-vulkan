#include <gtest/gtest.h>
#include "easy-vulkan.h"
#include "test_common.h"

class BuddyMemoryAllocatorTest : public ::testing::Test {
protected:
    std::shared_ptr<ev::Instance> instance;
    std::shared_ptr<ev::PhysicalDevice> physical_device;
    std::shared_ptr<ev::Device> device;

    void SetUp() override {
        create_default_test_context(instance, physical_device, device, true);
    }
};

TEST_F(BuddyMemoryAllocatorTest, CreateAndDestroyBuddyMemoryAllocator) {
    auto allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
    ASSERT_NE(allocator, nullptr);
    VkResult result = allocator->build();
    EXPECT_EQ(result, VK_SUCCESS);
    allocator.reset();
}

TEST_F(BuddyMemoryAllocatorTest, AddAndBuildMemoryPool) {
    auto allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
    ASSERT_NE(allocator, nullptr);
    allocator->add_pool(ev::memory_type::GPU_ONLY, 1024 * 1024); // 1MB
    VkResult result = allocator->build();
    EXPECT_EQ(result, VK_SUCCESS);
}

TEST_F(BuddyMemoryAllocatorTest, AllocateAndFreeBuffer) {
    auto allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
    ASSERT_NE(allocator, nullptr);
    allocator->add_pool(ev::memory_type::GPU_ONLY, 1024 * 1024); // 1MB
    VkResult result = allocator->build();
    EXPECT_EQ(result, VK_SUCCESS);

    auto buffer = std::make_shared<ev::Buffer>(device, 256 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    result = allocator->allocate_buffer(buffer, ev::memory_type::GPU_ONLY);
    EXPECT_EQ(result, VK_SUCCESS);

    buffer.reset();
}