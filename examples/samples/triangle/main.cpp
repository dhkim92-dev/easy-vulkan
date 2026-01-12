#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "base/example_base.hpp"

class TriangleExample : public ExampleBase {

private:

    vector<std::shared_ptr<ev::PipelineLayout>> pipeline_layouts;

    std::shared_ptr<ev::GraphicsPipeline> graphics_pipeline;

    std::shared_ptr<ev::PipelineCache> pipeline_cache;

    shared_ptr<ev::DescriptorPool> descriptor_pool;

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } ubo;

    struct {
        shared_ptr<ev::DescriptorSetLayout> layout;
        vector<shared_ptr<ev::DescriptorSet>> sets;
    } descriptors;

    struct GPUBuffer {;
        std::shared_ptr<ev::Buffer> buffer;
        std::shared_ptr<ev::Memory> memory;
    };

    struct {
        GPUBuffer vertex;
        GPUBuffer uniform;
    } buffers;

    // TriangleVertex vertices[3] = {
        // {glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f,ㅓ 0.0f, 0.0f)}, // Bottom left vertex
        // {glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}, // Bottom right vertex
        // {glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)}  // Top vertex
    // };

    struct TriangleVertex {
		float position[3];
		float color[3];
	};

    TriangleVertex vertices[3] = {
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
    };

    struct {
        std::shared_ptr<ev::Shader> vertex;
        std::shared_ptr<ev::Shader> fragment;
    } shaders;

    std::shared_ptr<ev::Fence> copy_complete_fence;

    VkPipeline pipeline;

public:

    explicit TriangleExample(std::string example_name, std::string executable_path, bool debug = false)
    : ExampleBase(example_name, executable_path, debug) {
        this->title = example_name;
        setup_default_context();
        setup_synchronize_objects();
        setup_uniform_buffer();
        setup_descriptor_sets();
        setup_pipeline_layouts();
        setup_shaders();
        setup_graphics_pipeline();
        setup_vertex_buffer();
    }

    void setup_uniform_buffer() {
        buffers.uniform.buffer = std::make_shared<ev::Buffer>(
            device, 
            sizeof(UniformBufferObject), 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        );
        buffers.uniform.memory = std::make_shared<ev::Memory>(
            device, 
            buffers.uniform.buffer->get_memory_requirements().size, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            buffers.uniform.buffer->get_memory_requirements()
        );
        buffers.uniform.buffer->bind_memory(buffers.uniform.memory, 0);
        // buffers.uniform.buffer->map();
    }

    void setup_synchronize_objects() {
        ev_log_info("[Setup Synchronization Objects Start]");
        copy_complete_fence = std::make_shared<ev::Fence>(device);
        ev_log_info("[Setup Synchronization Objects End]");
    }

    void create_memory_pool() override {
        // 이 예제에서는 메모리 풀을 사용하지 않습니다.
    }

    void setup_vertex_buffer() {
        ev_log_info("[Setup Vertex Buffers Start]");
        ev_log_debug("Setting up vertex buffer...");
        copy_complete_fence->reset();
        buffers.vertex.buffer = std::make_shared<ev::Buffer>(
            device, 
            sizeof(vertices), 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        );

        buffers.vertex.memory = std::make_shared<ev::Memory>(
            device, 
            buffers.vertex.buffer->get_memory_requirements().size, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            buffers.vertex.buffer->get_memory_requirements()
        );

        buffers.vertex.buffer->bind_memory(buffers.vertex.memory, 0);

        shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            sizeof(vertices), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        shared_ptr<ev::Memory> staging_memory = std::make_shared<ev::Memory>(
            device, 
            staging_buffer->get_memory_requirements().size, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            staging_buffer->get_memory_requirements()
        );
        CHECK_RESULT(staging_buffer->bind_memory(staging_memory, 0));
        CHECK_RESULT(staging_buffer->map());
        CHECK_RESULT(staging_buffer->write(vertices, sizeof(vertices)));
        CHECK_RESULT(staging_buffer->flush());
        CHECK_RESULT(staging_buffer->unmap());

        shared_ptr<ev::CommandBuffer> staging_command = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        staging_command->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        staging_command->copy_buffer(buffers.vertex.buffer, staging_buffer, sizeof(vertices));
        staging_command->end();
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, copy_complete_fence, nullptr));
        copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        staging_buffer.reset();
        staging_memory.reset();

        ev_log_debug("Vertex buffer created successfully.");
        ev_log_info("[Setup Vertex Buffers End]");
    }

    void record_command_buffers() {
        // sync_objects.frame_fences[current_frame_index]->reset();
        command_buffers[current_buffer_index]->reset();
        CHECK_RESULT(command_buffers[current_buffer_index]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT));
        command_buffers[current_buffer_index]->begin_render_pass(render_pass, 
            framebuffers[current_buffer_index], 
            { VkClearValue{ .color = { { 0.0f, 0.0f, 0.2f, 1.0f } } }, VkClearValue{ .depthStencil = { 1.0f, 0 } }}, 
            VK_SUBPASS_CONTENTS_INLINE);
        command_buffers[current_buffer_index]->set_viewport(0.0f, 0.0f, static_cast<float>(display.width), static_cast<float>(display.height));
        command_buffers[current_buffer_index]->set_scissor(0, 0, display.width, display.height);
        command_buffers[current_buffer_index]->bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts[0], {descriptors.sets[current_buffer_index]});
        
        command_buffers[current_buffer_index]->bind_graphics_pipeline(graphics_pipeline);
        command_buffers[current_buffer_index]->bind_vertex_buffers(0, {buffers.vertex.buffer}, {0});
        command_buffers[current_buffer_index]->draw(3, 1, 0, 0);
        // vkCmdDraw(VkCommandBuffer(*command_buffers[current_buffer_index]), 3, 1, 0, 0);
        command_buffers[current_buffer_index]->end_render_pass();
        CHECK_RESULT(command_buffers[current_buffer_index]->end());
    }

    void setup_descriptor_sets() {
        ev_log_debug("[Setup Descriptor Sets Start]");
        descriptor_pool = std::make_shared<ev::DescriptorPool>(device);
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3);
        CHECK_RESULT(descriptor_pool->create_pool(3, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT));
        ev_log_debug("Descriptor pool created");
        descriptors.layout = std::make_shared<ev::DescriptorSetLayout>(device);
        descriptors.layout->add_binding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1);
        CHECK_RESULT(descriptors.layout->create_layout());
        ev_log_debug("Descriptor set layout created");

        for (uint32_t i = 0 ; i < swapchain->get_images().size(); ++i) {
            descriptors.sets.push_back(descriptor_pool->allocate(descriptors.layout));
            descriptors.sets.back()->write_buffer(
                0, 
                buffers.uniform.buffer
            );
            CHECK_RESULT(descriptors.sets.back()->update());
        }
    }

    void setup_shaders() {
        vector<uint32_t> vertex_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "triangle.vert.spv").string().c_str(), vertex_shader_code);
        shaders.vertex = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_code);
        vector<uint32_t> fragment_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "triangle.frag.spv").string().c_str(), fragment_shader_code);
        shaders.fragment = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_code);
    }

    void setup_pipeline_layouts() {
        // vector<std::shared_ptr<ev::DescriptorSetLayout>> descriptor_set_layouts;
        std::vector<shared_ptr<ev::DescriptorSetLayout>> descriptor_set_layouts = {descriptors.layout};
        std::shared_ptr<ev::PipelineLayout> pipeline_layout 
            = std::make_shared<ev::PipelineLayout>(device, descriptor_set_layouts);
        pipeline_layouts.push_back(pipeline_layout);
    }

    void create_renderpass() override {
        VkAttachmentDescription color_attachment = {};
        color_attachment.format = swapchain->get_image_format();
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depth_attachment = {};
        depth_attachment.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        vector<VkAttachmentDescription> attachments = {color_attachment, depth_attachment};
        vector<VkSubpassDescription> subpasses;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        VkAttachmentReference color_reference = {};
        color_reference.layout= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_reference.attachment = 0; // Index of the color attachment
        subpass.pColorAttachments = &color_reference;
        VkAttachmentReference depth_reference = {};
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_reference.attachment = 1; // Index of the depth attachment
        subpass.pDepthStencilAttachment = &depth_reference;
        subpasses.push_back(subpass);
        
        vector<VkSubpassDependency> dependencies(2);
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependencies[1].dependencyFlags = 0;

        render_pass = std::make_shared<ev::RenderPass>(
            device, 
            attachments,
            subpasses,
            dependencies
        );
    }


    void setup_graphics_pipeline() {
        pipeline_cache = std::make_shared<ev::PipelineCache>(device);
        ev::GraphicsPipelineBluePrintManager blueprint_manager(device, render_pass);
        vector<shared_ptr<ev::GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
            .add_shader_stage(shaders.vertex)
            .add_shader_stage(shaders.fragment)
            .set_vertex_input_state()
            .add_vertex_input_binding_description(0, sizeof(float) * 6, VK_VERTEX_INPUT_RATE_VERTEX)
            .add_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .add_vertex_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3)
            .set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_rasterization_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .set_dynamic_state()
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .add_viewport(display.width, display.height, 0.0f, 1.0f)
            .add_scissor(0, 0, display.width, display.height)
            .set_multisample_state(VK_SAMPLE_COUNT_1_BIT)
            .set_color_blend_state()
            .add_color_blend_attachment_state(VK_FALSE)
            .set_depth_stencil_state(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
            .set_pipeline_layout(pipeline_layouts[0])
            .end_blueprint()
            .create_pipelines(pipeline_cache);
        graphics_pipeline = pipelines[0];
    }

    void uniform_update() {
        // 삼각형이 더 작게 보이도록 scale 적용
        ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
        // 카메라를 z=2 위치에서 원점(0,0,0)을 바라보게 함, up은 +Y
        ubo.view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 2.0f), // eye
            glm::vec3(0.0f, 0.0f, 0.0f), // center
            glm::vec3(0.0f, 1.0f, 0.0f)  // up
        );
        // 일반적인 원근 투영, fovy 45도, aspect, near=0.1, far=10.0
        ubo.proj = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(display.width) / static_cast<float>(display.height),
            0.1f, 10.0f
        );
        ubo.proj[1][1] *= -1; // Vulkan NDC 보정
        ev_log_debug("Updating uniform buffer with model, view, and projection matrices");
        CHECK_RESULT(buffers.uniform.buffer->map());
        CHECK_RESULT(buffers.uniform.buffer->write(&ubo, sizeof(ubo)));
        CHECK_RESULT(buffers.uniform.buffer->flush());
        CHECK_RESULT(buffers.uniform.buffer->unmap());
    }

    void render() override {
        
        prepare_frame();
        record_command_buffers();
        uniform_update();
        submit_frame();
    }

    void pre_destroy() override {
        // Cleanup code before destruction
        ev_log_debug("Pre-destroy cleanup for Triangle example");
        queue->wait_idle(UINT64_MAX);
        ev_log_debug("Pre-destroy cleanup for Triangle example complete");
    }
};

RUN_EXAMPLE_MAIN(TriangleExample, "triangle", false)
