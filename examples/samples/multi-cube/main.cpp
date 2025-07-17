#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "base/example_base.hpp"


class MultiCubeExample : public ExampleBase {

private:

    std::vector<std::shared_ptr<ev::PipelineLayout>> pipeline_layouts;

    std::shared_ptr<ev::RenderPass> render_pass;

    std::shared_ptr<ev::GraphicsPipeline> graphics_pipeline;

    std::shared_ptr<ev::PipelineCache> pipeline_cache;

    std::vector<std::shared_ptr<ev::Framebuffer>> framebuffers;

    uint32_t current_frame_index = 0;

    std::shared_ptr<ev::CommandPool> command_pool;

    std::vector<std::shared_ptr<ev::CommandBuffer>> command_buffers;
    
    std::shared_ptr<ev::Queue> queue;

    std::shared_ptr<ev::BitmapBuddyMemoryAllocator> memory_allocator;

    struct Camera {
        glm::mat4 view;
        glm::mat4 proj;
    } camera;

    struct {
        std::shared_ptr<ev::DescriptorPool> pool;
        std::shared_ptr<ev::DescriptorSetLayout> layout;
        std::vector<std::shared_ptr<ev::DescriptorSet>> sets;
    } descriptors;


    struct {
        std::shared_ptr<ev::Buffer> vertices;
        std::shared_ptr<ev::Buffer> indices;
        std::shared_ptr<ev::Buffer> camera_uniform;
        std::shared_ptr<ev::Buffer> instance_buffer;
        std::shared_ptr<ev::Texture> texture;
    } d_buffers;

    struct Cube {
        glm::mat4 model;
    };

    std::vector<Cube> cubes;

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 tex_coord;
        Vertex(glm::vec3 position, glm::vec2 tex_coord)
        : pos(position), tex_coord(tex_coord) {}
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
    
    const std::vector<Vertex> h_vertices = {
        // 각 면의 정점 위치에 맞게 UV 좌표를 (0,0) ~ (1,1) 사이로 매핑합니다.
        // Y축이 위쪽인 오른손 좌표계를 기준으로 합니다.

        // 앞면 (z=0.5)
        {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec2(0.0f, 0.0f)}, // 0 좌하
        {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec2(1.0f, 0.0f)},  // 1 우하
        {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec2(1.0f, 1.0f)},   // 2 우상
        {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec2(0.0f, 1.0f)},  // 3 좌상

        // 뒷면 (z=-0.5)
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f)}, // 4 
        {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f)}, // 5
        {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec2(0.0f, 1.0f)}, // 6
        {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec2(1.0f, 1.0f)}, // 7

        // 왼쪽 (x=-0.5)
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f)}, // 8
        {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec2(1.0f, 0.0f)}, // 9
        {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec2(1.0f, 1.0f)}, // 10
        {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec2(0.0f, 1.0f)}, // 11

        // 오른쪽 (x=0.5)
        {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f)}, // 12
        {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec2(0.0f, 0.0f)},  // 13
        {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec2(0.0f, 1.0f)},   // 14
        {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec2(1.0f, 1.0f)},  // 15

        // 아래쪽 (y=-0.5)
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 1.0f)}, // 16
        {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 1.0f)},  // 17
        {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec2(1.0f, 0.0f)},   // 18
        {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec2(0.0f, 0.0f)},  // 19

        // 위쪽 (y=0.5)
        {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec2(0.0f, 0.0f)}, // 20
        {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec2(1.0f, 0.0f)},  // 21
        {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec2(1.0f, 1.0f)},   // 22
        {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec2(0.0f, 1.0f)}   // 23
    };

    // 인덱스는 각 면의 4개 정점을 사용하여 2개의 삼각형을 만듭니다.
    const std::vector<uint32_t> h_indices = {
        1, 0, 2,   3, 2, 0,       // 앞면
        4, 5, 6,   7, 4, 6,       // 뒷면
        9, 8, 10,  11, 10, 8,      // 왼쪽
        13, 14, 12, 15, 12, 14,     // 오른쪽
        17, 16, 18, 19, 18, 16,     // 아래쪽
        20, 21, 22, 22, 23, 20      // 위쪽
    };

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

    explicit MultiCubeExample(std::string example_name, std::string executable_path, bool debug = true)
    : ExampleBase(example_name, executable_path, debug) {
        this->title = example_name;

        /** Host side resources **/
        setup_camera();
        setup_cubes();

        /** Device side resources **/
        setup_default_context();
        setup_synchronize_objects();
        setup_depth_stencil();
        setup_queue();
        setup_command_buffers();
        setup_memory_pool();
        setup_vertex_buffer();
        setup_instance_buffer();
        setup_texture();
        setup_uniform_buffer();
        setup_descriptor_sets();
        setup_pipeline_layouts();
        setup_shaders();
        setup_render_pass();
        setup_framebuffers();
        setup_graphics_pipeline();
    }

    void setup_texture() {
        ev::logger::Logger::getInstance().info("[Setup Texture Start]");
        uint8_t* texture_data = nullptr;
        int width = 0,height = 0, channels = 0, comp_req=0;
        std::string texture_path = resource_path / "textures" / "cube" / "wood.png";

        texture_data = stbi_load(texture_path.c_str(), &width, &height, &channels, comp_req);

        fprintf(stdout, "[std_image.h] Texture loaded: %s, Width: %d, Height: %d, Channels: %d\n", 
                texture_path.c_str(), width, height, channels);

        if (!texture_data) {
            throw std::runtime_error("Failed to load texture image");
        }

        std::shared_ptr<ev::Image> image = std::make_shared<ev::Image>(
            device,
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8G8B8A8_SRGB,
            width, height, 1
        );

        memory_allocator->allocate_image(image, ev::memory_type::GPU_ONLY);

        std::shared_ptr<ev::ImageView> image_view = std::make_shared<ev::ImageView>(
            device, 
            image, 
            VK_IMAGE_VIEW_TYPE_2D, 
            VK_FORMAT_R8G8B8A8_SRGB
        );

        std::shared_ptr<ev::Sampler> sampler = std::make_shared<ev::Sampler>(
            device, 
            VK_FILTER_LINEAR, 
            VK_FILTER_LINEAR, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
        );

        d_buffers.texture = std::make_shared<ev::Texture>(
            image, 
            image_view, 
            sampler
        );

        std::shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            width * height * 4, 
            ev::buffer_type::STAGING_BUFFER
        );
        memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE);

        staging_buffer->map();
        staging_buffer->write(texture_data, width * height * 4);
        // staging_buffer->flush();
        staging_buffer->unmap();

        shared_ptr<ev::CommandBuffer> staging_command = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        staging_command->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);


        VkImageSubresourceRange subresource_range = {};
        subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = 1;
        subresource_range.baseArrayLayer = 0;
        subresource_range.layerCount = 1;

        std::vector<ev::ImageMemoryBarrier> image_barriers;
        image_barriers.emplace_back( ev::ImageMemoryBarrier(
            d_buffers.texture->image,
            VK_ACCESS_NONE_KHR, 
            VK_ACCESS_TRANSFER_WRITE_BIT, 
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, 
            subresource_range
        ));

        staging_command->pipeline_barrier(
            VK_PIPELINE_STAGE_HOST_BIT, 
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            image_barriers,
            {}, 
            {}
        );

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.layerCount = 1;
        region.imageSubresource.baseArrayLayer = 0;

        staging_command->copy_buffer_to_image(d_buffers.texture->image, 
            staging_buffer,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            {region}
        );

        std::vector<ev::ImageMemoryBarrier> post_copy_barriers;
        post_copy_barriers.reserve(1);
        post_copy_barriers.emplace_back( ev::ImageMemoryBarrier(
            d_buffers.texture->image,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        ));
        staging_command->pipeline_barrier(
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
            post_copy_barriers,
            {}, 
            {}
        );
        staging_command->end();
        sync_objects.copy_complete_fence->reset();
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, sync_objects.copy_complete_fence, nullptr));
        sync_objects.copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        staging_buffer.reset();
        d_buffers.texture->image->transient_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        stbi_image_free(texture_data);
        ev::logger::Logger::getInstance().info("[Setup Texture End]");
    }

    void setup_memory_pool() {
        ev::logger::Logger::getInstance().info("[Setup Memory Pool Start]");
        memory_allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
        memory_allocator->add_pool(ev::memory_type::GPU_ONLY, 64*MB); 
        memory_allocator->add_pool(ev::memory_type::HOST_READABLE, 8*MB);
        CHECK_RESULT(memory_allocator->build());
        ev::logger::Logger::getInstance().info("[Setup Memory Pool End]");
    }

    void setup_uniform_buffer() {
        ev::logger::Logger::getInstance().info("[Setup Uniform Buffer Start]");
        d_buffers.camera_uniform= std::make_shared<ev::Buffer>(
            device, 
            sizeof(Camera),
            ev::buffer_type::UNIFORM_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.camera_uniform, ev::memory_type::HOST_READABLE);
        d_buffers.camera_uniform->map();
        d_buffers.camera_uniform->write(&camera, sizeof(Camera));
        d_buffers.camera_uniform->flush();
        d_buffers.camera_uniform->unmap();
        VkBuffer uniform_buffer_handle = *d_buffers.camera_uniform;
        ev::logger::Logger::getInstance().debug("Camera uniform buffer handler : " + std::to_string(reinterpret_cast<uintptr_t>(uniform_buffer_handle)));
        ev::logger::Logger::getInstance().info("[Setup Uniform Buffer End]");
    }

    void setup_instance_buffer() {
        ev::logger::Logger::getInstance().info("[Setup Instance Buffer Start]");
        std::shared_ptr<ev::CommandBuffer> staging_command = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        staging_command->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        ev::logger::Logger::getInstance().debug("Setting up instance buffer... with size : " + std::to_string(cubes.size()));

        size_t buffer_size = cubes.size() * sizeof(Cube);

        std::shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            buffer_size, 
            ev::buffer_type::STAGING_BUFFER
        );
        memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE);

        staging_buffer->map();
        staging_buffer->write(cubes.data(), buffer_size);
        staging_buffer->flush();
        staging_buffer->unmap();

        d_buffers.instance_buffer = std::make_shared<ev::Buffer>(
            device, 
            buffer_size,
            ev::buffer_type::VERTEX_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.instance_buffer, ev::memory_type::GPU_ONLY);
        staging_command->copy_buffer(d_buffers.instance_buffer, staging_buffer, buffer_size);
        staging_command->end();
        sync_objects.copy_complete_fence->reset();
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, sync_objects.copy_complete_fence, nullptr));
        sync_objects.copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        staging_buffer.reset();

        ev::logger::Logger::getInstance().debug("Instance buffer handler : " + std::to_string(reinterpret_cast<uintptr_t>(VkBuffer(*d_buffers.instance_buffer))));
        ev::logger::Logger::getInstance().info("[Setup Instance Buffer End]");
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
        d_buffers.vertices = std::make_shared<ev::Buffer>(
            device, 
            h_vertices.size() * sizeof(Vertex), 
            ev::buffer_type::VERTEX_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.vertices, ev::memory_type::GPU_ONLY);

        d_buffers.indices = std::make_shared<ev::Buffer>(
            device, 
            h_indices.size() * sizeof(uint32_t), 
            ev::buffer_type::INDEX_BUFFER
        );

        memory_allocator->allocate_buffer(d_buffers.indices, ev::memory_type::GPU_ONLY);

        shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            h_vertices.size() * sizeof(Vertex), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );

        memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE);
        staging_buffer->map();
        staging_buffer->write((void*)h_vertices.data(), h_vertices.size() * sizeof(Vertex));
        // staging_buffer->flush();
        staging_buffer->unmap();
        auto index_staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            h_indices.size() * sizeof(uint32_t), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        memory_allocator->allocate_buffer(index_staging_buffer, ev::memory_type::HOST_READABLE);
        index_staging_buffer->map();
        index_staging_buffer->write((void*)h_indices.data(), h_indices.size() * sizeof(uint32_t));
        // index_staging_buffer->flush();
        index_staging_buffer->unmap();

        shared_ptr<ev::CommandBuffer> staging_command = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        staging_command->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        staging_command->copy_buffer(d_buffers.vertices, staging_buffer, h_vertices.size() * sizeof(Vertex));
        staging_command->copy_buffer(d_buffers.indices, index_staging_buffer, h_indices.size() * sizeof(uint32_t));
        staging_command->end();
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, sync_objects.copy_complete_fence, nullptr));
        sync_objects.copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        staging_buffer.reset();
        index_staging_buffer.reset();
        

        ev::logger::Logger::getInstance().debug("Vertex buffer created successfully.");
        ev::logger::Logger::getInstance().debug("Vertex buffer handle: " + std::to_string(reinterpret_cast<uintptr_t>(VkBuffer(*d_buffers.vertices))));
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
        command_buffers[current_frame_index]->bind_vertex_buffers(0, {d_buffers.vertices, d_buffers.instance_buffer}, {0, 0});
        command_buffers[current_frame_index]->bind_index_buffers({d_buffers.indices}, 0, VK_INDEX_TYPE_UINT32);
        command_buffers[current_frame_index]->draw_indexed(h_indices.size(), cubes.size(), 0, 0, 0);
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
        descriptors.pool = std::make_shared<ev::DescriptorPool>(device);
        auto& descriptor_pool = descriptors.pool;
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 * cubes.size());
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3);
        CHECK_RESULT(descriptor_pool->create_pool(3, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT));
        ev::logger::Logger::getInstance().debug("Descriptor pool created");
        descriptors.layout = std::make_shared<ev::DescriptorSetLayout>(device);
        descriptors.layout->add_binding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1); // Camera uniform buffer
        descriptors.layout->add_binding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1); // Cube texture sampler
        CHECK_RESULT(descriptors.layout->create_layout());
        ev::logger::Logger::getInstance().debug("Descriptor set layout created");

        for (uint32_t i = 0 ; i < swapchain->get_images().size(); ++i) {
            descriptors.sets.push_back(descriptor_pool->allocate(descriptors.layout));
            ev::logger::Logger::getInstance().debug("Descriptor set " + std::to_string(i) + " allocated");
            descriptors.sets.back()->write_buffer(
                0,
                d_buffers.camera_uniform,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            );
            ev::logger::Logger::getInstance().debug("Descriptor set " + std::to_string(i) + "Camera Perspective View uniform buffer written");

            descriptors.sets.back()->write_texture(
                1, 
                d_buffers.texture,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            );
            ev::logger::Logger::getInstance().debug("Descriptor set " + std::to_string(i) + " texture written");
            CHECK_RESULT(descriptors.sets.back()->update());
            ev::logger::Logger::getInstance().debug("Descriptor set " + std::to_string(i) + " created");
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
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "multi-cube.vert.spv").c_str(), vertex_shader_code);
        shaders.vertex = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_code);
        vector<uint32_t> fragment_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "multi-cube.frag.spv").c_str(), fragment_shader_code);
        shaders.fragment = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_code);
    }

    void setup_pipeline_layouts() {
        // vector<std::shared_ptr<ev::DescriptorSetLayout>> descriptor_set_layouts;
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
            .add_vertex_input_binding_description(1, sizeof(Cube), VK_VERTEX_INPUT_RATE_INSTANCE)
            .add_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos))
            .add_vertex_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coord))
            .add_vertex_attribute_description(1, 2, VK_FORMAT_R32G32B32A32_SFLOAT,  offsetof(Cube, model) + sizeof(glm::vec4) * 0 )
            .add_vertex_attribute_description(1, 3, VK_FORMAT_R32G32B32A32_SFLOAT,  offsetof(Cube, model) + sizeof(glm::vec4) * 1 )
            .add_vertex_attribute_description(1, 4, VK_FORMAT_R32G32B32A32_SFLOAT,  offsetof(Cube, model) + sizeof(glm::vec4) * 2 )
            .add_vertex_attribute_description(1, 5, VK_FORMAT_R32G32B32A32_SFLOAT,  offsetof(Cube, model) + sizeof(glm::vec4) * 3 )
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

    void setup_camera() {
        camera.view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 8.0f), // position
            glm::vec3(0.0f, 0.0f, 0.0f), // center
            glm::vec3(0.0f, -1.0f, 0) // up vector
        );
        camera.proj = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(display.width) / static_cast<float>(display.height),
            0.1f, 100.0f
        );
    }

    void setup_cubes() {
        std::vector<glm::vec3> cube_positions = {
            glm::vec3( 0.0f,  0.0f,  0.0f),
            glm::vec3( 2.0f,  5.0f, -15.0f),
            glm::vec3(-1.5f, -2.2f, -2.5f),
            glm::vec3(-3.8f, -2.0f, -12.3f),
            glm::vec3( 2.4f, -0.4f, -3.5f),
            glm::vec3(-1.7f,  3.0f, -7.5f),
            glm::vec3( 1.3f, -2.0f, -2.5f),
            glm::vec3( 1.5f,  2.0f, -2.5f),
            glm::vec3( 1.5f,  0.2f, -1.5f),
            glm::vec3(-1.3f,  1.0f, -1.5f)
        };

        cubes.resize(10);

        for ( size_t i = 0 ; i < cubes.size() ; ++i ) {
            cubes[i].model = glm::mat4(1.0f);
            cubes[i].model = glm::translate(cubes[i].model, cube_positions[i]);
            cubes[i].model = glm::rotate(cubes[i].model, glm::radians(20.0f * i), glm::vec3(1.0f, 0.3f, 0.5f));
            cubes[i].model = glm::scale(cubes[i].model, glm::vec3(1.0f, 1.0f, 1.0f));
        }
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

        // ev::logger::Logger::getInstance().debug("Current frame index: " + std::to_string(current_frame_index));
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
        // ev::logger::Logger::getInstance().debug("Current frame index updated to: " + std::to_string(current_frame_index));
        uniform_update();
            // exit(EXIT_FAILURE);
     }



    void on_window_resize() {
        // Handle window resize
        // swapchain->create(swapchain->get_surface(), display.width, display.height);
        ev::logger::Logger::getInstance().debug("Window resized to: " + std::to_string(display.width) + "x" + std::to_string(display.height));
    }
};

RUN_EXAMPLE_MAIN(MultiCubeExample, "multi-cube")