#include <gtest/gtest.h>
#include <memory>
#include <easy-vulkan.h>
#include "test_common.h"

class SamplerTest : public ::testing::Test {
protected:
    std::shared_ptr<ev::Instance> instance;
    std::shared_ptr<ev::PhysicalDevice> physical_device;
    std::shared_ptr<ev::Device> device;

    void SetUp() override {
        create_default_test_context(instance, physical_device, device, true);
    }
};

TEST_F(SamplerTest, CreateAndDestroySampler) {
    // Create a sampler with default parameters
    shared_ptr<ev::Sampler> sampler = make_shared<ev::Sampler>(
        device,
        VK_FILTER_LINEAR, // mag_filter
        VK_FILTER_LINEAR, // min_filter
        VK_SAMPLER_ADDRESS_MODE_REPEAT, // address_mode_u
        VK_SAMPLER_ADDRESS_MODE_REPEAT, // address_mode_v
        VK_SAMPLER_ADDRESS_MODE_REPEAT, // address_mode_w
        VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, // border_color
        0.0f, // mip_lod_bias
        1.0f, // max_anisotropy
        false, // unnormalized_coordinates
        false, // compare_enable
        VK_COMPARE_OP_ALWAYS, // compare_op
        0.0f, // min_lod
        1.0f, // max_lod
        0, // flags
        nullptr // next
    );

    EXPECT_NE(VkSampler(*sampler), VK_NULL_HANDLE);

    // Destroy the sampler and check if it is nullified
    sampler->destroy();
    EXPECT_EQ(VkSampler(*sampler), VK_NULL_HANDLE);
}
