#include <gtest/gtest.h>
#include "test_common.h"
#include "ev-sync.h"

using namespace ev;

class SemaphoreTest : public ::testing::Test {
protected:
    std::shared_ptr<Instance> instance;
    std::shared_ptr<PhysicalDevice> physical_device;
    std::shared_ptr<Device> device;

    void SetUp() override {
        // 기본 테스트 컨텍스트 생성
        create_default_test_context(instance, physical_device, device, true);
    }
};

TEST_F(SemaphoreTest, CreateAndDestroy) {
    ev::logger::Logger::getInstance().debug("Creating semaphore...");
    Semaphore semaphore(device);
    EXPECT_NE(VkSemaphore(semaphore), VK_NULL_HANDLE) << "Failed to create semaphore";
    // Semaphore 파괴
    semaphore.destroy();
}

