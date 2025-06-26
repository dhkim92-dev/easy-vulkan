
#include <gtest/gtest.h>
#include <easy-vulkan.h>
#include <memory>

using namespace std;
using namespace ev;

class RenderPassTest : public ::testing::Test {
protected:
    shared_ptr<Instance> instance;
    shared_ptr<PhysicalDevice> physical_device;
    shared_ptr<Device> device;

    void SetUp() override {
        ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::ERROR);
        instance = make_shared<ev::Instance>(vector<const char*>(), vector<const char*>(), false);
        physical_device = make_shared<ev::PhysicalDevice>(instance, ev::utility::list_physical_devices(instance->get_instance())[0]);
        device = make_shared<ev::Device>(instance, physical_device, vector<const char*>(), VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
    }
};

TEST_F(RenderPassTest, CreateRenderPass) {
    vector<VkAttachmentDescription> attachments = {
        {
            0, // flags
            VK_FORMAT_B8G8R8A8_UNORM, // format
            VK_SAMPLE_COUNT_1_BIT, // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR, // loadOp
            VK_ATTACHMENT_STORE_OP_STORE, // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // finalLayout
        }
    };

    vector<VkSubpassDescription> subpasses = {
        {
            0, // flags
            VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
            0, nullptr, // inputAttachments
            0, nullptr, // colorAttachments
            nullptr, // resolveAttachments
            nullptr, // depthStencilAttachment
            0, nullptr // preserveAttachments
        }
    };

    RenderPass render_pass(device, attachments, subpasses);
    EXPECT_NE(VkRenderPass(render_pass), VK_NULL_HANDLE);
}