#include <gtest/gtest.h>
#include <easy-vulkan.h>

using namespace std;
using namespace ev;

class FramebufferTest : public ::testing::Test {
protected:
    shared_ptr<Instance> instance;
    shared_ptr<PhysicalDevice> physical_device;
    shared_ptr<Device> device;
    shared_ptr<RenderPass> render_pass;
    shared_ptr<Image> image;
    shared_ptr<Memory> memory;
    shared_ptr<ImageView> attachment;

    void SetUp() override {
        ev::logger::Logger::getInstance().set_log_level(ev::logger::LogLevel::DEBUG);
	vector<const char*> instance_extension = {VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
	vector<const char*> layers = {"VK_LAYER_KHRONOS_validation"};
	vector<const char*> device_extension = {"VK_KHR_swapchain"};
        instance = make_shared<ev::Instance>(instance_extension, layers, true);
        physical_device = make_shared<ev::PhysicalDevice>(instance, ev::utility::list_physical_devices(instance->get_instance())[0]);
        device = make_shared<ev::Device>(instance, physical_device, device_extension, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

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

        render_pass = make_shared<ev::RenderPass>(device, attachments, subpasses);
        image = make_shared<ev::Image>(device, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
        memory = make_shared<ev::Memory>(device, image->get_memory_requirements().size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image->get_memory_requirements(), nullptr);
	image->bind_memory(memory, 0);
        attachment = make_shared<ev::ImageView>(device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_VIEW_TYPE_2D);
    }
};


TEST_F(FramebufferTest, CreateFramebuffer) {
    vector<shared_ptr<ImageView>> attachments = { attachment };
    ev::Framebuffer framebuffer(device, render_pass, attachments, 800, 600);
    EXPECT_NE(VkFramebuffer(framebuffer), VK_NULL_HANDLE);
    EXPECT_EQ(framebuffer.get_width(), 800);
    EXPECT_EQ(framebuffer.get_height(), 600);
}
