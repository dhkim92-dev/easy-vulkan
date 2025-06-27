#include <gtest/gtest.h>
#include "easy-vulkan.h"

using namespace ev;

class QueueTest : public ::testing::Test {
protected:
    std::shared_ptr<Instance> instance;
    std::shared_ptr<PhysicalDevice> physical_device;
    std::shared_ptr<Device> device;

    void SetUp() override {
        vector<const char*> extensions = {};
        logger::Logger::getInstance().set_log_level(logger::LogLevel::ERROR);
        instance = std::make_shared<Instance>( extensions, extensions, false );
        physical_device = std::make_shared<PhysicalDevice>(instance, instance->get_physical_devices()[0]);
        device = std::make_shared<Device>(instance, physical_device, extensions);
    }
};

TEST_F(QueueTest, CreateQueue) {

    ev::Queue queue(device, device->get_queue_index(VK_QUEUE_GRAPHICS_BIT), 0);
    EXPECT_NE(VkQueue(queue), VK_NULL_HANDLE) << "Queue should be created successfully.";
}
