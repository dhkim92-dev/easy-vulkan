#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "base/example_base.hpp"

class PhongShadingExample : public ExampleBase {

private:

    // vector<std::shared_ptr<ev::Shader>> shaders;

    vector<std::shared_ptr<ev::PipelineLayout>> pipeline_layouts;

    std::shared_ptr<ev::RenderPass> render_pass;

    std::shared_ptr<ev::GraphicsPipeline> graphics_pipeline;

    std::shared_ptr<ev::PipelineCache> pipeline_cache;

    std::vector<std::shared_ptr<ev::Framebuffer>> framebuffers;

    uint32_t current_frame_index = 0;

    std::shared_ptr<ev::CommandPool> command_pool;

    shared_ptr<ev::DescriptorPool> descriptor_pool;

    std::vector<std::shared_ptr<ev::CommandBuffer>> command_buffers;
    
    std::shared_ptr<ev::Queue> queue;

    std::shared_ptr<ev::BitmapBuddyMemoryAllocator> memory_allocator;

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

    struct {
        std::shared_ptr<ev::Image> image;
        std::shared_ptr<ev::Memory> memory;
        std::shared_ptr<ev::ImageView> view;
    } depth_stencil;

    struct {
        std::shared_ptr<ev::Fence> copy_complete_fence;
        std::vector<std::shared_ptr<ev::Semaphore>> render_complete_semaphore;
        std::vector<std::shared_ptr<ev::Semaphore>> present_complete_semaphore;
        std::vector<shared_ptr<ev::Fence>> frame_fences;
    } sync_objects;

    VkPipeline pipeline;

    float current_frame_time = 0.0f;

    float last_frame_time = 0.0f;

public:

    explicit PhongShadingExample(std::string example_name, std::string executable_path, bool debug = false)
    : ExampleBase(example_name, executable_path, debug) {
        this->title = example_name;
        setup_default_context();
        setup_synchronize_objects();
        setup_depth_stencil();
        setup_command_buffers();
        setup_memory_pool();
        setup_sphere();
        setup_uniform_data();
        setup_uniform_buffer();
        setup_descriptor_sets();
        setup_pipeline_layouts();
        setup_shaders();
        setup_render_pass();
        setup_framebuffers();
        setup_graphics_pipeline();
        setup_queue();
        setup_vertex_buffer();
    }

    void setup_sphere() {
        generate_sphere(1.0f, 36, 18);
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
        ev::logger::Logger::getInstance().info("[Setup Camera Start]");
        ubo.pos = glm::vec4(5.0f, -5.0f, 5.0f, 1.0f); // Light position
        ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(display.width) / static_cast<float>(display.height), 0.1f, 100.0f);
        ubo.view = glm::mat4(1.0f);
        ubo.view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 10.0f), // position
            glm::vec3(0.0f, 0.0f, 0.0f), // center
            glm::vec3(0.0f, 1.0f, 0.0f) // up vector
        );
        ev::logger::Logger::getInstance().info("[Setup Camera End]");
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
        ev::logger::Logger::getInstance().info("[Setup Uniform Buffer End]");
    }

    void setup_synchronize_objects() {
        ev::logger::Logger::getInstance().info("[Setup Synchronization Objects Start]");
        sync_objects.copy_complete_fence = std::make_shared<ev::Fence>(device);
        for (uint32_t i = 0 ; i < swapchain->get_images().size(); ++i) {
            sync_objects.render_complete_semaphore.push_back(std::make_shared<ev::Semaphore>(device));
            sync_objects.present_complete_semaphore.push_back(std::make_shared<ev::Semaphore>(device));
            sync_objects.frame_fences.push_back(std::make_shared<ev::Fence>(device));
        }
        ev::logger::Logger::getInstance().info("[Setup Synchronization Objects End]");
    }

    void setup_queue() {
        ev::logger::Logger::getInstance().debug("[Setup Queue Start]");
        queue = std::make_shared<ev::Queue>(device, device->get_queue_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT));
        ev::logger::Logger::getInstance().debug("[Setup Queue End]");
    }

    void setup_vertex_buffer() {
        ev::logger::Logger::getInstance().info("[Setup Vertex Buffers Start]");
        ev::logger::Logger::getInstance().debug("Setting up vertex buffer...");
        // ev::logger::Logger::getInstance().debug("Vertex buffer size: " + std::to_string(sizeof(vertices)) + " bytes");
        sync_objects.copy_complete_fence->reset();
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

        ev::logger::Logger::getInstance().debug("[SphereExample::setup_vertex_buffer] Creating vertex buffer with size: " 
            + std::to_string(d_buffers.sphere_vertices->get_size()) 
            + " bytes with vk handle : " + std::to_string(reinterpret_cast<uintptr_t>(VkBuffer(*d_buffers.sphere_vertices))));

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
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, sync_objects.copy_complete_fence, nullptr));
        sync_objects.copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        staging_buffer.reset();
        ev::logger::Logger::getInstance().debug("Vertex buffer created successfully.");
        ev::logger::Logger::getInstance().debug("Vertex buffer handle: " + std::to_string(reinterpret_cast<uintptr_t>(VkBuffer(*d_buffers.sphere_vertices))));
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
        command_buffers[current_frame_index]->bind_vertex_buffers(0, {d_buffers.sphere_vertices}, {0});
        command_buffers[current_frame_index]->bind_index_buffers({d_buffers.sphere_indices}, 0, VK_INDEX_TYPE_UINT32);
        command_buffers[current_frame_index]->draw_indexed(static_cast<uint32_t>(h_buffers.sphere_indices.size()), 1, 0, 0);
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
        descriptors.layout->add_binding(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1);
        CHECK_RESULT(descriptors.layout->create_layout());
        ev::logger::Logger::getInstance().debug("Descriptor set layout created");

        for (uint32_t i = 0 ; i < swapchain->get_images().size(); ++i) {
            descriptors.sets.push_back(descriptor_pool->allocate(descriptors.layout));
            descriptors.sets.back()->write_buffer(
                0, 
                d_buffers.camera_uniform
            );
            CHECK_RESULT(descriptors.sets.back()->update());
        }
        ev::logger::Logger::getInstance().debug("Descriptor sets created: " + std::to_string(descriptors.sets.size()));
    }

    void setup_depth_stencil() {
        depth_stencil.image = std::make_shared<ev::Image>(
            device, 
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            display.width, 
            display.height, 
            1,1,1,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL
        );
        depth_stencil.memory = std::make_shared<ev::Memory>(
            device,
            depth_stencil.image->get_memory_requirements().size,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depth_stencil.image->get_memory_requirements()
        );
        depth_stencil.image->bind_memory(depth_stencil.memory);

        VkComponentMapping component_mapping = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        VkImageSubresourceRange subresource_ranges = {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1};
        
        depth_stencil.view = std::make_shared<ev::ImageView>(
            device,
            depth_stencil.image,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            component_mapping,
            subresource_ranges
        );
    }

    void setup_shaders() {
        vector<uint32_t> vertex_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "sphere.vert.spv").c_str(), vertex_shader_code);
        shaders.vertex = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_code);
        vector<uint32_t> fragment_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "sphere.frag.spv").c_str(), fragment_shader_code);
        shaders.fragment = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_code);
    }

    void setup_pipeline_layouts() {
        std::vector<shared_ptr<ev::DescriptorSetLayout>> descriptor_set_layouts = {descriptors.layout};
        std::shared_ptr<ev::PipelineLayout> pipeline_layout 
            = std::make_shared<ev::PipelineLayout>(device, descriptor_set_layouts);
        pipeline_layouts.push_back(pipeline_layout);
    }

    void setup_render_pass() {
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

    void setup_framebuffers() {
        framebuffers.clear();
        // sync_objects.frame_fences.clear();
        ev::logger::Logger::getInstance().debug("Creating framebuffers for swapchain images... with size : " + std::to_string(swapchain->get_image_views().size()));
        for (const auto& image_view : swapchain->get_image_views()) {
            vector<VkImageView> attachments = {image_view, *depth_stencil.view};
            std::shared_ptr<ev::Framebuffer> framebuffer = std::make_shared<ev::Framebuffer>(
                device, 
                render_pass, 
                attachments,
                display.width,
                display.height,
                1
            );
            framebuffers.push_back(framebuffer);
            // sync_objects.frame_fences.push_back(make_shared<ev::Fence>(device));
        }
        ev::logger::Logger::getInstance().debug("Framebuffers created successfully with size : " + std::to_string(framebuffers.size()));

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
    }

    void uniform_update() {
        if ( last_frame_time == 0.0f ) {
            last_frame_time = current_frame_time = glfwGetTime();
        } else {
            current_frame_time = glfwGetTime();
        }
        last_frame_time = current_frame_time;
    }

    void render() override {
        // exit(EXIT_SUCCESS); // Exit immediately for testing purposes
        sync_objects.frame_fences[current_frame_index]->wait();
        CHECK_RESULT(sync_objects.frame_fences[current_frame_index]->reset());

        uint32_t image_index = 0;

        VkResult result = swapchain->acquire_next_image(image_index, sync_objects.present_complete_semaphore[current_frame_index]);
        if ( result == VK_ERROR_OUT_OF_DATE_KHR ) {
            on_window_resize();
            return;
        } else if ( (result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR) ) {
            ev::logger::Logger::getInstance().error("Failed to acquire next image from swapchain: " + std::to_string(result));
            exit(EXIT_FAILURE);
        }

        ev::logger::Logger::getInstance().debug("Current frame index: " + std::to_string(current_frame_index));
        record_command_buffers();
        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        queue->submit(command_buffers[current_frame_index], 
            {sync_objects.present_complete_semaphore[current_frame_index]},
            {sync_objects.render_complete_semaphore[image_index]},
            &waitStageMask,
            sync_objects.frame_fences[current_frame_index]
        );
        queue->wait_idle(UINT64_MAX);
        // queue->wait_idle(UINT64_MAX);
        result = queue->present(swapchain, current_frame_index, {sync_objects.render_complete_semaphore[current_frame_index]});

        if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
            on_window_resize();
		}
		else if (result != VK_SUCCESS) {
            ev::logger::Logger::getInstance().error("Failed to present image to swapchain: " + std::to_string(result));
            exit(EXIT_FAILURE);
		}

        current_frame_index = (current_frame_index + 1) % swapchain->get_images().size();
        ev::logger::Logger::getInstance().debug("Current frame index updated to: " + std::to_string(current_frame_index));
        uniform_update();
     }



    void on_window_resize() {
        // Handle window resize
        // swapchain->create(swapchain->get_surface(), display.width, display.height);
        ev::logger::Logger::getInstance().debug("Window resized to: " + std::to_string(display.width) + "x" + std::to_string(display.height));
    }
};

RUN_EXAMPLE_MAIN(PhongShadingExample, "phong-shading")