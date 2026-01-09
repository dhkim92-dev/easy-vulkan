#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "base/example_base.hpp"

class ExampleImpl : public ExampleBase {

private:

    // vector<std::shared_ptr<ev::Shader>> shaders;

    std::vector<std::shared_ptr<ev::PipelineLayout>> pipeline_layouts;

    std::shared_ptr<ev::GraphicsPipeline> graphics_pipeline;

    std::shared_ptr<ev::PipelineCache> pipeline_cache;

    std::shared_ptr<ev::CommandPool> command_pool;

    std::shared_ptr<ev::DescriptorPool> descriptor_pool;

    // std::vector<std::shared_ptr<ev::CommandBuffer>> command_buffers;

    std::shared_ptr<ev::BitmapBuddyMemoryAllocator> memory_allocator;

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } ubo;

    struct {
        std::shared_ptr<ev::DescriptorSetLayout> layout;
        std::vector<std::shared_ptr<ev::DescriptorSet>> sets;
    } descriptors;


    struct {
        std::shared_ptr<ev::Buffer> cube_vertices;
        std::shared_ptr<ev::Buffer> cube_indices;
        std::shared_ptr<ev::Buffer> uniform;
    } buffers;

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        Vertex(glm::vec3 position, glm::vec3 normal_vector)
            : pos(position), normal(normal_vector) {}
    };

    // Cube의 8개 꼭지점 좌표, Normal 값은 해당 좌표에서 0,0,0으로 향하는 벡터의 역방향, 즉 좌표값과 동일
    // NDC는 -y가 위쪽, +y가 아래쪽으로 설정
    //    v4 -------v5
    //   / |       / |
    //  /  |      /  | 
    // v0-------v1   |
    // |  v7 - - | - v6
    // | /       |  /
    // |         | /
    // v3-------v2
    std::vector<Vertex> cube_vertices = {
        {glm::vec3(-0.5f, -0.5f, 0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, 0.5f))}, // v0 전면 좌측 상단 꼭지점
        {glm::vec3(0.5f, -0.5f, 0.5f), glm::normalize(glm::vec3(0.5f, -0.5f, 0.5f))}, // v1 전면 우측 상단 꼭지점
        {glm::vec3(0.5f, 0.5f, 0.5f), glm::normalize(glm::vec3(0.5f, 0.5f, 0.5f))}, // v2 전면 우측 하단 꼭지점
        {glm::vec3(-0.5f, 0.5f, 0.5f), glm::normalize(glm::vec3(-0.5f, 0.5f, 0.5f))}, // v3 전면 좌측 하단 꼭지점
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f))}, // v4 후면 좌측 상단 꼭지점
        {glm::vec3(0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(0.5f, -0.5f, -0.5f))}, // v5 후면 우측 상단 꼭지점
        {glm::vec3(0.5f, 0.5f, -0.5f), glm::normalize(glm::vec3(0.5f, 0.5f, -0.5f))}, // v6 후면 우측 하단 꼭지점
        {glm::vec3(-0.5f, 0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, 0.5f, -0.5f))} // v7 후면 좌측 하단 꼭지점
    };

    // CCW 순서로 Cube의 12개의 삼각형을 구성하는 인덱스
std::vector<uint32_t> cube_indices = {
    // 앞면 (+Z) — Vulkan에서 정면 바라볼 때 CCW: v0, v3, v2,  v2, v1, v0
    0, 3, 2,
    2, 1, 0,

    // 오른쪽 면 (+X) — CCW: v1, v2, v6,  v6, v5, v1
    1, 2, 6,
    6, 5, 1,

    // 뒷면 (-Z) — CCW: v5, v6, v7,  v7, v4, v5
    5, 6, 7,
    7, 4, 5,

    // 왼쪽 면 (-X) — CCW: v4, v7, v3,  v3, v0, v4
    4, 7, 3,
    3, 0, 4,

    // 윗면 (+Y) — Vulkan에선 아래가 +Y라서 윗면은 시각적으로 아래쪽. CCW: v3, v7, v6,  v6, v2, v3
    3, 7, 6,
    6, 2, 3,

    // 아랫면 (-Y) — 시각적으로 위쪽. CCW: v4, v0, v1,  v1, v5, v4
    4, 0, 1,
    1, 5, 4
};

    struct {
        std::shared_ptr<ev::Shader> vertex;
        std::shared_ptr<ev::Shader> fragment;
    } shaders;

    struct {
        std::shared_ptr<ev::Fence> copy_complete_fence;
        std::vector<shared_ptr<ev::Fence>> frame_fences;
    } sync_objects;

    VkPipeline pipeline;

    float current_frame_time = 0.0f;

    float last_frame_time = 0.0f;

public:

    explicit ExampleImpl(std::string example_name, std::string executable_path, bool debug = false)
    : ExampleBase(example_name, executable_path, debug) {
        this->title = example_name;
        setup_default_context();
        setup_command_buffers();
        setup_memory_pool();
        setup_uniform_buffer();
        setup_descriptor_sets();
        setup_pipeline_layouts();
        setup_shaders();
        setup_synchronize_objects();
        setup_graphics_pipeline();
        setup_vertex_buffer();
    }

    void setup_memory_pool() {
        ev::logger::Logger::getInstance().info("[Setup Memory Pool Start]");
        memory_allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
        memory_allocator->add_pool(ev::memory_type::GPU_ONLY, 64*MB); 
        memory_allocator->add_pool(ev::memory_type::HOST_READABLE, 1*MB);
        CHECK_RESULT(memory_allocator->build());
        ev::logger::Logger::getInstance().info("[Setup Memory Pool End]");
    }

    void setup_uniform_buffer() {
        ev::logger::Logger::getInstance().info("[Setup Uniform Buffer Start]");
        buffers.uniform= std::make_shared<ev::Buffer>(
            device, 
            sizeof(UniformBufferObject), 
            ev::buffer_type::UNIFORM_BUFFER
        );
        memory_allocator->allocate_buffer(buffers.uniform, ev::memory_type::HOST_READABLE);
        buffers.uniform->map();
        buffers.uniform->write(&ubo, sizeof(ubo));
        buffers.uniform->flush();
        buffers.uniform->unmap();
        ev::logger::Logger::getInstance().info("[Setup Uniform Buffer End]");
    }

    void setup_synchronize_objects() {
        ev::logger::Logger::getInstance().info("[Setup Synchronization Objects Start]");
        sync_objects.copy_complete_fence = std::make_shared<ev::Fence>(device);
        for (uint32_t i = 0 ; i < swapchain->get_images().size(); ++i) {
            sync_objects.frame_fences.push_back(std::make_shared<ev::Fence>(device));
        }
        ev::logger::Logger::getInstance().info("[Setup Synchronization Objects End]");
    }

    void setup_vertex_buffer() {
        ev::logger::Logger::getInstance().info("[Setup Vertex Buffers Start]");
        ev::logger::Logger::getInstance().debug("Setting up vertex buffer...");
        // ev::logger::Logger::getInstance().debug("Vertex buffer size: " + std::to_string(sizeof(vertices)) + " bytes");
        sync_objects.copy_complete_fence->reset();
        buffers.cube_vertices= std::make_shared<ev::Buffer>(
            device, 
            cube_vertices.size() * sizeof(Vertex), 
            ev::buffer_type::VERTEX_BUFFER
        );
        memory_allocator->allocate_buffer(buffers.cube_vertices, ev::memory_type::GPU_ONLY);

        ev::logger::Logger::getInstance().debug("[CubeExample::setup_vertex_buffer] Creating vertex buffer with size: " + std::to_string(sizeof(cube_vertices)) + " bytes with vk handle : " + std::to_string(reinterpret_cast<uintptr_t>(VkBuffer(*buffers.cube_vertices))));

        buffers.cube_indices = std::make_shared<ev::Buffer>(
            device, 
            cube_indices.size() * sizeof(uint32_t), 
            ev::buffer_type::INDEX_BUFFER
        );
        memory_allocator->allocate_buffer(buffers.cube_indices, ev::memory_type::GPU_ONLY);

        shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            cube_vertices.size() * sizeof(Vertex), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );

        memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE);
        staging_buffer->map();
        staging_buffer->write(cube_vertices.data(), cube_vertices.size() * sizeof(Vertex));
        // staging_buffer->flush();
        staging_buffer->unmap();
        auto index_staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            cube_indices.size() * sizeof(uint32_t), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        memory_allocator->allocate_buffer(index_staging_buffer, ev::memory_type::HOST_READABLE);
        index_staging_buffer->map();
        index_staging_buffer->write(cube_indices.data(), cube_indices.size() * sizeof(uint32_t));
        // index_staging_buffer->flush();
        index_staging_buffer->unmap();

        shared_ptr<ev::CommandBuffer> staging_command = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        staging_command->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        staging_command->copy_buffer(buffers.cube_vertices, staging_buffer, cube_vertices.size() * sizeof(Vertex));
        staging_command->copy_buffer(buffers.cube_indices, index_staging_buffer, cube_indices.size() * sizeof(uint32_t));
        staging_command->end();
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, sync_objects.copy_complete_fence, nullptr));
        sync_objects.copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        staging_buffer.reset();
        index_staging_buffer.reset();
        

        ev::logger::Logger::getInstance().debug("Vertex buffer created successfully.");
        ev::logger::Logger::getInstance().debug("Vertex buffer handle: " + std::to_string(reinterpret_cast<uintptr_t>(VkBuffer(*buffers.cube_vertices))));
        ev::logger::Logger::getInstance().info("[Setup Vertex Buffers End]");
    }

    void record_command_buffers() {
        // sync_objects.frame_fences[current_frame_index]->reset();
        command_buffers[current_frame_index]->reset();
        CHECK_RESULT(command_buffers[current_frame_index]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT));
        command_buffers[current_frame_index]->begin_render_pass(render_pass, 
            framebuffers[current_frame_index], 
            { VkClearValue{ .color = { { 0.0f, 0.0f, 0.2f, 1.0f } } }, VkClearValue{ .depthStencil = { 1.0f, 0 } }}, 
            VK_SUBPASS_CONTENTS_INLINE);
        command_buffers[current_frame_index]->set_viewport(0.0f, 0.0f, static_cast<float>(display.width), static_cast<float>(display.height));
        command_buffers[current_frame_index]->set_scissor(0, 0, display.width, display.height);
        command_buffers[current_frame_index]->bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts[0], {descriptors.sets[current_frame_index]});
        
        command_buffers[current_frame_index]->bind_graphics_pipeline(graphics_pipeline);
        command_buffers[current_frame_index]->bind_vertex_buffers(0, {buffers.cube_vertices}, {0});
        command_buffers[current_frame_index]->bind_index_buffers({buffers.cube_indices}, 0, VK_INDEX_TYPE_UINT32);
        command_buffers[current_frame_index]->draw_indexed(cube_indices.size(), 1, 0, 0, 0);
        command_buffers[current_frame_index]->end_render_pass();
        CHECK_RESULT(command_buffers[current_frame_index]->end());
    }

    void setup_command_buffers() {
        command_pool = std::make_shared<ev::CommandPool>(device, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
        command_buffers.resize(swapchain->get_images().size());
        for (size_t i = 0; i < command_buffers.size(); ++i){
            command_buffers[i] = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        }
    }

    void setup_descriptor_sets() {
        ev::logger::Logger::getInstance().debug("[Setup Descriptor Sets Start]");
        descriptor_pool = std::make_shared<ev::DescriptorPool>(device);
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3);
        CHECK_RESULT(descriptor_pool->create_pool(3, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT));
        ev::logger::Logger::getInstance().debug("Descriptor pool created");
        descriptors.layout = std::make_shared<ev::DescriptorSetLayout>(device);
        descriptors.layout->add_binding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1);
        CHECK_RESULT(descriptors.layout->create_layout());
        ev::logger::Logger::getInstance().debug("Descriptor set layout created");

        for (uint32_t i = 0 ; i < swapchain->get_images().size(); ++i) {
            descriptors.sets.push_back(descriptor_pool->allocate(descriptors.layout));
            descriptors.sets.back()->write_buffer(
                0, 
                buffers.uniform
            );
            CHECK_RESULT(descriptors.sets.back()->update());
        }
        ev::logger::Logger::getInstance().debug("Descriptor sets created: " + std::to_string(descriptors.sets.size()));
    }

    void setup_shaders() {
        vector<uint32_t> vertex_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "cube.vert.spv").c_str(), vertex_shader_code);
        shaders.vertex = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_code);
        vector<uint32_t> fragment_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "cube.frag.spv").c_str(), fragment_shader_code);
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
        std::printf("Creating render pass...\n");
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

        std::printf("Renderpass handler : %lu\n", reinterpret_cast<uintptr_t>(VkRenderPass(*render_pass)));
    }

    void setup_graphics_pipeline() {
        pipeline_cache = std::make_shared<ev::PipelineCache>(device);
        ev::GraphicsPipelineBluePrintManager blueprint_manager(device, render_pass);
        vector<shared_ptr<ev::GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
            .add_shader_stage(shaders.vertex)
            .add_shader_stage(shaders.fragment)
            .set_vertex_input_state()
            .add_vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
            .add_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .add_vertex_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3)
            .set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_rasterization_state(VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
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
        if ( last_frame_time == 0.0f ) {
            // last_frame_time = current_frame_time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            last_frame_time = current_frame_time = glfwGetTime();
        } else {
            // current_frame_time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            current_frame_time = glfwGetTime();
        }
        // 사각형이 매 프레임마다 회전하도록 설정
        ubo.model = glm::scale( glm::rotate(glm::mat4(1.0f), glm::radians(current_frame_time * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.5f, 0.5f, 0.5f));
        // 카메라를 z=2 위치에서 원점(0,0,0)을 바라보게 함, up은 +Y
        ubo.view = glm::lookAt(
            glm::vec3(0.0f, 1.0f, 2.0f), // eye
            glm::vec3(0.0f, 0.0f, 0.0f), // center
            glm::vec3(0.0f, -1.0f, 0.0f)  // up
        );
        // 일반적인 원근 투영, fovy 45도, aspect, near=0.1, far=10.0
        ubo.proj = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(display.width) / static_cast<float>(display.height),
            0.1f, 10.0f
        );
        // ubo.proj[1][1] *= -1; // Vulkan NDC 보정
        buffers.uniform->map();
        buffers.uniform->write(&ubo, sizeof(ubo));
        // buffers.uniform.buffer->flush();
        buffers.uniform->unmap();
        last_frame_time = current_frame_time;
    }

    void render() override {
        // sync_objects.frame_fences[current_frame_index]->wait();
        // CHECK_RESULT(sync_objects.frame_fences[current_frame_index]->reset());
        prepare_frame();
        record_command_buffers();
        uniform_update();
        submit_frame();
    }
};

RUN_EXAMPLE_MAIN(ExampleImpl, "cube", false)