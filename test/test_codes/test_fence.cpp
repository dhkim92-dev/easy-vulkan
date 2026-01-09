#include <gtest/gtest.h>
#include "test_common.h"
#include "ev-sync.h"

using namespace ev;

class FenceTest: public ::testing::Test {
protected:
    std::shared_ptr<Instance> instance;
    std::shared_ptr<PhysicalDevice> physical_device;
    std::shared_ptr<Device> device;

    void SetUp() override {
        // 기본 테스트 컨텍스트 생성
        create_default_test_context(instance, physical_device, device, true);
    }
};

TEST_F(FenceTest, CreateAndDestroy) {
    ev_log_debug("Creating fence...");
    Fence fence(device);
    EXPECT_NE(VkFence(fence), VK_NULL_HANDLE) << "Failed to create fence";
    fence.destroy();
}

