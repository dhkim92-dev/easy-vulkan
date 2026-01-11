#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "easy-vulkan.h"

using namespace std;

using namespace ev;

class DescriptorLayerTests : public ::testing::Test {
protected:
    std::shared_ptr<Instance> instance;
    std::shared_ptr<PhysicalDevice> physical_device;
    std::shared_ptr<Device> device;

    void SetUp() override {
        vector<const char*> extensions = {};
        instance = std::make_shared<Instance>(extensions, extensions, false);
        physical_device = std::make_shared<PhysicalDevice>(instance, instance->get_physical_devices()[0]);
        device = std::make_shared<Device>(instance, physical_device, extensions);
    }
};

TEST_F(DescriptorLayerTests, Create_DescriptorSetLayout_And_Destroy) {
    auto descriptor_set_layout = std::make_shared<ev::DescriptorSetLayout>(device);
    EXPECT_NO_THROW({
        descriptor_set_layout->add_binding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
        descriptor_set_layout->create_layout();
    }) << "DescriptorSetLayout should be created successfully.";

    EXPECT_NO_THROW({
        descriptor_set_layout->destroy();
    }) << "DescriptorSetLayout should be destroyed successfully.";
    descriptor_set_layout.reset(); // Ensure the layout is destroyed
}

TEST_F(DescriptorLayerTests, Create_Pool_And_Destroy) {
    auto descriptor_pool = std::make_shared<ev::DescriptorPool>(device);
    EXPECT_NO_THROW({
        descriptor_pool->create_pool();
    }) << "DescriptorPool should be created successfully.";

    EXPECT_NO_THROW({
        descriptor_pool->destroy();
    }) << "DescriptorPool should be destroyed successfully.";
    descriptor_pool.reset(); // Ensure the pool is destroyed
}

TEST_F(DescriptorLayerTests, Allocate_DescriptorSet) {
    shared_ptr<ev::DescriptorSetLayout> descriptor_set_layout = std::make_shared<ev::DescriptorSetLayout>(device);
    descriptor_set_layout->add_binding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
    descriptor_set_layout->create_layout();

    auto descriptor_pool = std::make_shared<ev::DescriptorPool>(device);
    descriptor_pool->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2);
    descriptor_pool->create_pool();

    shared_ptr<ev::DescriptorSet> descriptor_set = descriptor_pool->allocate(descriptor_set_layout);

    descriptor_set_layout->destroy();
}

TEST_F(DescriptorLayerTests, Release_DescriptorSet) {
    auto layout = std::make_shared<ev::DescriptorSetLayout>(device);
    layout->add_binding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
    layout->create_layout();
    auto pool = std::make_shared<ev::DescriptorPool>(device);
    pool->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2);
    pool->create_pool();
    auto descriptor_set = pool->allocate(layout);;;;

    EXPECT_EQ(pool->release(descriptor_set), VK_SUCCESS) << "DescriptorSet should be released successfully.";
}