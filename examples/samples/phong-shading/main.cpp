#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "base/example_base.hpp"

class ExampleImpl : public ExampleBase {

private:

    // vector<std::shared_ptr<ev::Shader>> shaders;

    vector<std::shared_ptr<ev::PipelineLayout>> pipeline_layouts;

    std::shared_ptr<ev::GraphicsPipeline> graphics_pipeline;

    std::shared_ptr<ev::PipelineCache> pipeline_cache;

    shared_ptr<ev::DescriptorPool> descriptor_pool;

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        Vertex(glm::vec3 position, glm::vec3 normal_vector)
            : pos(position), normal(normal_vector) {}
    };

    struct UBO {
        glm::mat4 view; // View matrix
        glm::mat4 proj; // Projection matrix
        glm::vec4 pos; // Light Position
    } ubo;

    struct {
        shared_ptr<ev::DescriptorSetLayout> layout;
        vector<shared_ptr<ev::DescriptorSet>> sets;
    } descriptors;

    struct {
        std::shared_ptr<ev::Buffer> sphere_vertices;
        std::shared_ptr<ev::Buffer> sphere_indices;
        std::shared_ptr<ev::Buffer> camera_uniform;
    } d_buffers;

    struct {
        std::vector<Vertex> sphere_vertices;
        std::vector<uint32_t> sphere_indices;
    } h_buffers;
    
    struct {
        std::shared_ptr<ev::Shader> vertex;
        std::shared_ptr<ev::Shader> fragment;
    } shaders;

    std::shared_ptr<ev::Fence> copy_complete_fence;

    VkPipeline pipeline;

    float current_frame_time = 0.0f;

    float last_frame_time = 0.0f;

public:

    explicit ExampleImpl(std::string example_name, std::string executable_path, bool debug = false)
    : ExampleBase(example_name, executable_path, debug) {
        this->title = example_name;
        setup_default_context();
        setup_synchronize_objects();
        generate_sphere(1.0f, 36, 18);
        setup_uniform_data();
        setup_uniform_buffer();
        setup_descriptor_sets();
        setup_pipeline_layouts();
        setup_shaders();
        setup_graphics_pipeline();
        setup_vertex_buffer();
    }

    void generate_sphere(float radius, uint32_t sector_count, uint32_t stack_count) {
        const float pi = glm::pi<float>();
        const float sector_step = 2 * pi / sector_count;
        const float stack_step = pi / stack_count;

        // 구체 상 정점 생성 
        for (uint32_t i = 0; i <= stack_count; ++i) {
            float stack_angle = pi / 2 - i * stack_step; // from pi/2 to -pi/2
            float xy = radius * cosf(stack_angle); // r * cos(u)
            float z = radius * sinf(stack_angle); // r * sin(u)

            for (uint32_t j = 0; j <= sector_count; ++j) {
                float sector_angle = j * sector_step; // from 0 to 2pi
                float x = xy * cosf(sector_angle); // r * cos(u) * cos(v)
                float y = xy * sinf(sector_angle); // r * cos(u) * sin(v)

                auto pos = glm::vec3(x, y, z);
                auto normal = glm::normalize(pos);

                h_buffers.sphere_vertices.emplace_back(pos, normal);
            }
        }

        // 삼각형 인덱스 생성
        for (uint32_t i = 0; i < stack_count; ++i) {
        uint32_t k1 = i * (sector_count + 1);
        uint32_t k2 = k1 + sector_count + 1;

        for (uint32_t j = 0; j < sector_count; ++j, ++k1, ++k2) {
                if (i != 0) {
                    // 위쪽 삼각형
                    h_buffers.sphere_indices.push_back(k1);
                    h_buffers.sphere_indices.push_back(k1 + 1);
                    h_buffers.sphere_indices.push_back(k2);
                }

                if (i != (stack_count - 1)) {
                    // 아래쪽 삼각형
                    h_buffers.sphere_indices.push_back(k1 + 1);
                    h_buffers.sphere_indices.push_back(k2 + 1);
                    h_buffers.sphere_indices.push_back(k2);
                }
            }
        }
    }

    void setup_uniform_data() {
        ev_log_info("[Setup Camera Start]");
        ubo.pos = glm::vec4(5.0f, -5.0f, 5.0f, 1.0f); // Light position
        ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(display.width) / static_cast<float>(display.height), 0.1f, 100.0f);
        ubo.view = glm::mat4(1.0f);
        ubo.view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 10.0f), // position
            glm::vec3(0.0f, 0.0f, 0.0f), // center
            glm::vec3(0.0f, 1.0f, 0.0f) // up vector
        );
        ev_log_info("[Setup Camera End]");
    }

    void create_memory_pool() override {
        ev_log_info("[Create Memory Pool Start]");
        memory_allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
        memory_allocator->add_pool(ev::memory_type::GPU_ONLY, 64*MB); 
        memory_allocator->add_pool(ev::memory_type::HOST_READABLE, 1*MB);
        CHECK_RESULT(memory_allocator->build());
        ev_log_info("[Create Memory Pool End]");
    }

    void setup_uniform_buffer() {
        ev_log_info("[Setup Uniform Buffer Start]");
        d_buffers.camera_uniform= std::make_shared<ev::Buffer>(
            device, 
            sizeof(UBO), 
            ev::buffer_type::UNIFORM_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.camera_uniform, ev::memory_type::HOST_READABLE);
        d_buffers.camera_uniform->map();
        d_buffers.camera_uniform->write(&ubo, sizeof(UBO));
        d_buffers.camera_uniform->flush();
        d_buffers.camera_uniform->unmap();
        ev_log_info("[Setup Uniform Buffer End]");
    }

    void setup_synchronize_objects() {
        ev_log_info("[Setup Synchronization Objects Start]");
        copy_complete_fence = std::make_shared<ev::Fence>(device);
        ev_log_info("[Setup Synchronization Objects End]");
    }

    void setup_vertex_buffer() {
        ev_log_info("[Setup Vertex Buffers Start]");
        ev_log_debug("Setting up vertex buffer...");
        // ev_log_debug("Vertex buffer size: " + std::to_string(sizeof(vertices)) + " bytes");
        copy_complete_fence->reset();
        d_buffers.sphere_vertices= std::make_shared<ev::Buffer>(
            device, 
            h_buffers.sphere_vertices.size() * sizeof(Vertex), 
            ev::buffer_type::VERTEX_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.sphere_vertices, ev::memory_type::GPU_ONLY);
        d_buffers.sphere_indices = std::make_shared<ev::Buffer>(
            device, 
            h_buffers.sphere_indices.size() * sizeof(uint32_t), 
            ev::buffer_type::INDEX_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.sphere_indices, ev::memory_type::GPU_ONLY);

        shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            h_buffers.sphere_vertices.size() * sizeof(Vertex), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );

        memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE);
        staging_buffer->map();
        staging_buffer->write(h_buffers.sphere_vertices.data(), h_buffers.sphere_vertices.size() * sizeof(Vertex));
        staging_buffer->flush();
        staging_buffer->unmap();

        std::shared_ptr<ev::Buffer> index_staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            h_buffers.sphere_indices.size() * sizeof(uint32_t), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        memory_allocator->allocate_buffer(index_staging_buffer, ev::memory_type::HOST_READABLE);
        index_staging_buffer->map();
        index_staging_buffer->write(h_buffers.sphere_indices.data(), h_buffers.sphere_indices.size() * sizeof(uint32_t));
        index_staging_buffer->flush();
        index_staging_buffer->unmap();

        shared_ptr<ev::CommandBuffer> staging_command = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        staging_command->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        staging_command->copy_buffer(d_buffers.sphere_vertices, staging_buffer, h_buffers.sphere_vertices.size() * sizeof(Vertex));
        staging_command->copy_buffer(d_buffers.sphere_indices, index_staging_buffer, h_buffers.sphere_indices.size() * sizeof(uint32_t), 0, 0);
        staging_command->end();
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, copy_complete_fence, nullptr));
        copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        staging_buffer.reset();
        ev_log_info("[Setup Vertex Buffers End]");
    }

    void record_command_buffers() {
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
        command_buffers[current_buffer_index]->bind_vertex_buffers(0, {d_buffers.sphere_vertices}, {0});
        command_buffers[current_buffer_index]->bind_index_buffers({d_buffers.sphere_indices}, 0, VK_INDEX_TYPE_UINT32);
        command_buffers[current_buffer_index]->draw_indexed(static_cast<uint32_t>(h_buffers.sphere_indices.size()), 1, 0, 0);
        command_buffers[current_buffer_index]->end_render_pass();
        CHECK_RESULT(command_buffers[current_buffer_index]->end());
    }

    void setup_descriptor_sets() {
        ev_log_info("[Setup Descriptor Sets Start]");
        descriptor_pool = std::make_shared<ev::DescriptorPool>(device);
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3);
        CHECK_RESULT(descriptor_pool->create_pool(3, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT));
        descriptors.layout = std::make_shared<ev::DescriptorSetLayout>(device);
        descriptors.layout->add_binding(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1);
        CHECK_RESULT(descriptors.layout->create_layout());

        for (uint32_t i = 0 ; i < swapchain->get_images().size(); ++i) {
            descriptors.sets.push_back(descriptor_pool->allocate(descriptors.layout));
            descriptors.sets.back()->write_buffer(
                0, 
                d_buffers.camera_uniform
            );
            CHECK_RESULT(descriptors.sets.back()->update());
        }
        ev_log_info("[Setup Descriptor Sets End]");
    }

    void setup_shaders() {
        ev_log_info("[Setup Shaders Start]");
        vector<uint32_t> vertex_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "sphere.vert.spv").string().c_str(), vertex_shader_code);
        shaders.vertex = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_code);
        vector<uint32_t> fragment_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "sphere.frag.spv").string().c_str(), fragment_shader_code);
        shaders.fragment = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_code);
        ev_log_info("[Setup Shaders End]");
    }

    void setup_pipeline_layouts() {
        ev_log_info("[Setup Pipeline layout Start]");
        std::vector<shared_ptr<ev::DescriptorSetLayout>> descriptor_set_layouts = {descriptors.layout};
        std::shared_ptr<ev::PipelineLayout> pipeline_layout 
            = std::make_shared<ev::PipelineLayout>(device, descriptor_set_layouts);
        pipeline_layouts.push_back(pipeline_layout);
        ev_log_info("[Setup Pipeline layout End]");
    }
    
    void setup_graphics_pipeline() {
        ev_log_info("[Setup Graphics Pipeline Start]");
        pipeline_cache = std::make_shared<ev::PipelineCache>(device);
        ev::GraphicsPipelineBluePrintManager blueprint_manager(device, render_pass);
        std::vector<shared_ptr<ev::GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
            .add_shader_stage(shaders.vertex)
            .add_shader_stage(shaders.fragment)
            .set_vertex_input_state()
            .add_vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
            .add_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .add_vertex_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3)
            .set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_rasterization_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
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
        ev_log_info("[Setup Graphics Pipeline End]");
    }

    void uniform_update() {
        if ( last_frame_time == 0.0f ) {
            last_frame_time = current_frame_time = glfwGetTime();
        } else {
            current_frame_time = glfwGetTime();
        }
        last_frame_time = current_frame_time;
    }

    void create_renderpass() override {
        ev_log_info("[Create Render Pass Start]");
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
        ev_log_info("[Create Render Pass End]");
    }


    void render() override {
        // exit(EXIT_SUCCESS); // Exit immediately for testing purposes
        prepare_frame();
        record_command_buffers();
        uniform_update();
        submit_frame();
    }
};

RUN_EXAMPLE_MAIN(ExampleImpl, "phong-shading", false)
