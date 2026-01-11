#include <gtest/gtest.h>
#include "easy-vulkan.h"
#include "test_common.h"

using namespace std;
using namespace ev;

class PipelineTest : public ::testing::Test {
protected:
    shared_ptr<Instance> instance;
    shared_ptr<PhysicalDevice> physical_device;
    shared_ptr<Device> device;
    shared_ptr<RenderPass> render_pass;

    void SetUp() override {
        vector<const char*> required_instance_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            #ifdef __APPLE__
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            #endif
        };
        vector<const char*> required_layers = {
            "VK_LAYER_KHRONOS_validation",
        };
        instance = make_shared<Instance>(required_instance_extensions, required_layers, true);
        physical_device = make_shared<PhysicalDevice>(instance, instance->get_physical_devices()[0]);
        device = make_shared<Device>(instance, physical_device, vector<const char*> {"VK_KHR_swapchain"}, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
        vector<VkAttachmentDescription> attachments = {{
            0, // flags
            VK_FORMAT_B8G8R8A8_UNORM, // format
            VK_SAMPLE_COUNT_1_BIT, // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR, // loadOp
            VK_ATTACHMENT_STORE_OP_STORE, // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // finalLayout
        }};

        vector<VkAttachmentReference> color_attachments = {{
            0, // attachment index
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
        }};

        vector<VkSubpassDescription> subpasses = {
            {
                0, // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
                0, nullptr, // inputAttachments
                1, color_attachments.data(), // colorAttachments
                nullptr, // resolveAttachments
                nullptr, // depthStencilAttachment
                0, nullptr // preserveAttachments
            }
        };
        render_pass = make_shared<RenderPass>(device, attachments, subpasses);
    }
};

TEST_F(PipelineTest, PipelineCacheCreateTest) {
    auto pipeline_cache = make_shared<PipelineCache>(device);
    EXPECT_NE(VkPipelineCache(*pipeline_cache), VK_NULL_HANDLE) << "PipelineCache should be created successfully.";
    
    pipeline_cache->destroy();
    EXPECT_EQ(VkPipelineCache(*pipeline_cache), VK_NULL_HANDLE) << "PipelineCache should be destroyed successfully.";
}

TEST_F(PipelineTest, PipelineLayoutCreateTest) {
    vector<shared_ptr<DescriptorSetLayout>> descriptor_set_layouts;
    vector<VkPushConstantRange> push_constant_ranges;

    auto pipeline_layout = make_shared<PipelineLayout>(device, descriptor_set_layouts, push_constant_ranges);
    EXPECT_NE(VkPipelineLayout(*pipeline_layout), VK_NULL_HANDLE) << "PipelineLayout should be created successfully.";

    pipeline_layout->destroy();
    EXPECT_EQ(VkPipelineLayout(*pipeline_layout), VK_NULL_HANDLE) << "PipelineLayout should be destroyed successfully.";
}

TEST_F(PipelineTest, PipelineLayoutWithPushConstantsCreateTest) {
    vector<shared_ptr<DescriptorSetLayout>> descriptor_set_layouts;
    vector<VkPushConstantRange> push_constant_ranges = {
        {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4}
    };

    auto pipeline_layout = make_shared<PipelineLayout>(device, descriptor_set_layouts, push_constant_ranges);
    EXPECT_NE(VkPipelineLayout(*pipeline_layout), VK_NULL_HANDLE) << "PipelineLayout with push constants should be created successfully.";

    pipeline_layout->destroy();
    EXPECT_EQ(VkPipelineLayout(*pipeline_layout), VK_NULL_HANDLE) << "PipelineLayout with push constants should be destroyed successfully.";
}

TEST_F(PipelineTest, GraphicsPipelineBlueprintManagerCreateTest) {
    GraphicsPipelineBluePrintManager blueprint_manager(device, render_pass);
    shared_ptr<DescriptorSetLayout> descriptor_set_layout = make_shared<DescriptorSetLayout>(device);
    descriptor_set_layout->create_layout();
    vector<shared_ptr<DescriptorSetLayout>> descriptor_set_layouts;
    std::shared_ptr<PipelineLayout> pipeline_layout = make_shared<PipelineLayout>(device,descriptor_set_layouts, vector<VkPushConstantRange>());
    shared_ptr<PipelineCache> pipeline_cache = make_shared<PipelineCache>(device, 0, nullptr);

    // Create a basic graphics pipeline blueprint
    std::filesystem::path base_path = get_executable_dir();
    auto vertex_shader_path =  base_path / "shaders" / "vert.spv";
    auto fragment_shader_path = base_path / "shaders" / "frag.spv";
    printf("Vertex Shader Path: %s\n", vertex_shader_path.c_str());
    printf("Fragment Shader Path: %s\n", fragment_shader_path.c_str());
    vector<uint32_t> vertex_shader_code;
    vector<uint32_t> fragment_shader_code; 

    utility::read_spirv_shader_file(
        vertex_shader_path.c_str(),
        vertex_shader_code  
    );

    EXPECT_FALSE(vertex_shader_code.empty()) << "Vertex shader code should not be empty.";

    utility::read_spirv_shader_file(
        fragment_shader_path.c_str(),
        fragment_shader_code
    );

    EXPECT_FALSE(fragment_shader_code.empty()) << "Fragment shader code should not be empty.";

    // auto input_binding_description = ev::initializer::vertex_input_binding_description(0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX);
    // auto input_attribute_description = ev::initializer::vertex_input_attribute_description(0, 0);

    vector<shared_ptr<GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
        .add_shader_stage(make_shared<Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_code))
        .add_shader_stage(make_shared<Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_code))
        .set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        // .add_vertex_input_binding_description(input_binding_description)
        // .add_vertex_attribute_description(input_attribute_description)
        .set_rasterization_state(VK_POLYGON_MODE_FILL)
        .set_pipeline_layout(pipeline_layout)
        .set_color_blend_state()
        .add_color_blend_attachment_state()
        .set_viewport_state()
        .add_viewport(800, 600, 0.0f, 1.0f)
        .add_scissor(0, 0, 800, 600)
        .set_multisample_state(VK_SAMPLE_COUNT_1_BIT)
        .set_depth_stencil_state(VK_FALSE, VK_FALSE)
        .set_dynamic_state()
        .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
        .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
        .set_pipeline_layout(pipeline_layout)
        .end_blueprint()
        .create_pipelines(pipeline_cache);

    EXPECT_NE(VkPipeline(*pipelines[0]), VK_NULL_HANDLE) << "Graphics Pipeline should be created successfully.";
    pipelines[0]->destroy();
    EXPECT_EQ(VkPipeline(*pipelines[0]), VK_NULL_HANDLE) << "Graphics Pipeline should be destroyed successfully.";
}