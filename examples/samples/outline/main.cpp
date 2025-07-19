#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "base/example_base.hpp"

class ExampleImpl : public ExampleBase {

private:

    bool first_draw = true;

    std::shared_ptr<ev::PipelineCache> pipeline_cache;

    std::shared_ptr<ev::CommandPool> command_pool;

    std::shared_ptr<ev::DescriptorPool> descriptor_pool;

    std::shared_ptr<ev::Queue> queue;

    std::shared_ptr<ev::BitmapBuddyMemoryAllocator> memory_allocator;

    struct {
        std::shared_ptr<ev::RenderPass> scene;
        std::shared_ptr<ev::RenderPass> outline;
        std::shared_ptr<ev::RenderPass> composite;
    } render_passes;

    struct {
        std::vector<std::shared_ptr<ev::Framebuffer>> scene;
        std::vector<std::shared_ptr<ev::Framebuffer>> outline;
        std::vector<std::shared_ptr<ev::Framebuffer>> composite;
    } framebuffers;

    struct FramebufferAttachment {
        std::shared_ptr<ev::Image> image;
        std::shared_ptr<ev::ImageView> view;
    };

    std::vector<FramebufferAttachment> framebuffer_attachments;

    struct CommandBuffers {
        std::vector<std::shared_ptr<ev::CommandBuffer>> scene;
        std::vector<std::shared_ptr<ev::CommandBuffer>> outline;
        std::vector<std::shared_ptr<ev::CommandBuffer>> composite;
    } commands_buffers;

    struct {
        std::vector<std::shared_ptr<ev::DescriptorSet>> scene;
        std::vector<std::shared_ptr<ev::DescriptorSet>> outline;
        std::vector<std::shared_ptr<ev::DescriptorSet>> composite;
        std::shared_ptr<ev::DescriptorSetLayout> scene_layout;
        std::shared_ptr<ev::DescriptorSetLayout> outline_layout;
        std::shared_ptr<ev::DescriptorSetLayout> composite_layout;
    } descriptors;

    struct { 
        std::shared_ptr<ev::GraphicsPipeline> scene;
        std::shared_ptr<ev::PipelineLayout> scene_layout;
        std::shared_ptr<ev::GraphicsPipeline> outline;
        std::shared_ptr<ev::PipelineLayout> outline_layout;
        std::shared_ptr<ev::GraphicsPipeline> composite;
        std::shared_ptr<ev::PipelineLayout> composite_layout;
        std::shared_ptr<ev::PipelineLayout> blur_layout;
    } pipelines;

    struct {
        std::shared_ptr<ev::Shader> scene_vert;
        std::shared_ptr<ev::Shader> scene_frag;
        std::shared_ptr<ev::Shader> outline_vert;
        std::shared_ptr<ev::Shader> outline_frag;
        std::shared_ptr<ev::Shader> composite_vert;
        std::shared_ptr<ev::Shader> composite_frag;
    } shaders;

    struct {
        std::shared_ptr<ev::Buffer> cube_vertices;
        std::shared_ptr<ev::Buffer> cube_indices;
        std::shared_ptr<ev::Texture> texture;
        std::shared_ptr<ev::Buffer> uniform;
        std::vector<std::shared_ptr<ev::Texture>> scene_texture;
        std::vector<std::shared_ptr<ev::Texture>> outline_texture;
        // std::vector<std::shared_ptr<ev::Sampler>> scene_sampler;
        // std::vector<std::shared_ptr<ev::Sampler>> outline_sampler;
        // std::shared_ptr<ev::Sampler> outline_stencil_sampler;
        // std::shared_ptr<ev::Sampler> composite_sampler;
    } d_buffers;

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 tex_coord;
        Vertex(glm::vec3 position, glm::vec3 normal, glm::vec2 tex_coord)
            : pos(position), normal(normal), tex_coord(tex_coord) {}
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
        // 각 면의 정점 위치에 맞게 UV 좌표를 (0,0) ~ (1,1) 사이로 매핑합니다.
        // Y축이 위쪽인 오른손 좌표계를 기준으로 합니다.
        // pos, normal, tex_coord
        // 앞면 (z=0.5)
        {glm::vec3(-0.5f, -0.5f, 0.5f), glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f)), glm::vec2(0.0f, 0.0f)}, // 0 좌하
        {glm::vec3(0.5f, -0.5f, 0.5f), glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)), glm::vec2(1.0f, 0.0f)},  // 1 우하
        {glm::vec3(0.5f, 0.5f, 0.5f), glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec2(1.0f, 1.0f)},   // 2 우상
        {glm::vec3(-0.5f, 0.5f, 0.5f), glm::normalize(glm::vec3(-1.0f, 1.0f, 1.0f)), glm::vec2(0.0f, 1.0f)},  // 3 좌상

        // 뒷면 (z=-0.5)
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f)), glm::vec2(1.0f, 0.0f)}, // 4 
        {glm::vec3(0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(0.5f, -0.5f, -0.5f)) , glm::vec2(0.0f, 0.0f)}, // 5
        {glm::vec3(0.5f, 0.5f, -0.5f), glm::normalize(glm::vec3(0.5f, 0.5f, -0.5f)), glm::vec2(0.0f, 1.0f)}, // 6
        {glm::vec3(-0.5f, 0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, 0.5f, -0.5f)), glm::vec2(1.0f, 1.0f)}, // 7

        // 왼쪽 (x=-0.5)
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f)), glm::vec2(0.0f, 0.0f)}, // 8
        {glm::vec3(-0.5f, -0.5f, 0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, 0.5f)), glm::vec2(1.0f, 0.0f)}, // 9
        {glm::vec3(-0.5f, 0.5f, 0.5f), glm::normalize(glm::vec3(-0.5f, 0.5f, 0.5f)) , glm::vec2(1.0f, 1.0f)}, // 10
        {glm::vec3(-0.5f, 0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, 0.5f, -0.5f)), glm::vec2(0.0f, 1.0f)}, // 11

        // 오른쪽 (x=0.5)
        {glm::vec3(0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(0.5f, -0.5f, -0.5f)), glm::vec2(1.0f, 0.0f)}, // 12
        {glm::vec3(0.5f, -0.5f, 0.5f), glm::normalize(glm::vec3(0.5f, -0.5f, 0.5f)), glm::vec2(0.0f, 0.0f)},  // 13
        {glm::vec3(0.5f, 0.5f, 0.5f), glm::normalize(glm::vec3(0.5f, 0.5f, 0.5f)),  glm::vec2(0.0f, 1.0f)},   // 14
        {glm::vec3(0.5f, 0.5f, -0.5f), glm::normalize(glm::vec3(0.5f, 0.5f, -0.5f)), glm::vec2(1.0f, 1.0f)},  // 15

        // 아래쪽 (y=-0.5)
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f)), glm::vec2(0.0f, 1.0f)}, // 16
        {glm::vec3(0.5f, -0.5f, -0.5f), glm::normalize(glm::vec3(0.5f, -0.5f, -0.5f)), glm::vec2(1.0f, 1.0f)},  // 17
        {glm::vec3(0.5f, -0.5f, 0.5f), glm::normalize(glm::vec3(0.5f, -0.5f, 0.5f)), glm::vec2(1.0f, 0.0f)},   // 18
        {glm::vec3(-0.5f, -0.5f, 0.5f), glm::normalize(glm::vec3(-0.5f, -0.5f, 0.5f)), glm::vec2(0.0f, 0.0f)},  // 19

        // 위쪽 (y=0.5)
        {glm::vec3(-0.5f, 0.5f, -0.5f), glm::normalize(glm::vec3(-0.5f, 0.5f, -0.5f)), glm::vec2(0.0f, 0.0f)}, // 20
        {glm::vec3(0.5f, 0.5f, -0.5f), glm::normalize(glm::vec3(0.5f, 0.5f, -0.5f)), glm::vec2(1.0f, 0.0f)},  // 21
        {glm::vec3(0.5f, 0.5f, 0.5f), glm::normalize(glm::vec3(0.5f, 0.5f, 0.5f)), glm::vec2(1.0f, 1.0f)},   // 22
        {glm::vec3(-0.5f, 0.5f, 0.5f), glm::normalize(glm::vec3(-0.5f, 0.5f, 0.5f)), glm::vec2(0.0f, 1.0f)}   // 23
    };

    // 인덱스는 각 면의 4개 정점을 사용하여 2개의 삼각형을 만듭니다.
    std::vector<uint32_t> cube_indices = {
        1, 0, 2,   3, 2, 0,       // 앞면
        4, 5, 6,   7, 4, 6,       // 뒷면
        9, 8, 10,  11, 10, 8,      // 왼쪽
        13, 14, 12, 15, 12, 14,     // 오른쪽
        17, 16, 18, 19, 18, 16,     // 아래쪽
        20, 21, 22, 22, 23, 20      // 위쪽
    };

    struct UniformBuffer {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } ubo;

    struct {
        std::shared_ptr<ev::Fence> copy_complete_fence;
        std::vector<std::shared_ptr<ev::Semaphore>> render_complete_semaphore;
        std::vector<std::shared_ptr<ev::Semaphore>> present_complete_semaphore;
        std::vector<shared_ptr<ev::Fence>> frame_fences;
    } sync_objects;

    uint32_t current_frame_index = 0;

    float current_frame_time = 0.0f;

    float last_frame_time = 0.0f;

public:

    explicit ExampleImpl(std::string example_name, std::string executable_path, bool debug = true)
    : ExampleBase(example_name, executable_path, debug) {
        this->title = example_name;
        setup_default_context();
        setup_synchronize_objects();
        setup_queue();
        setup_command_pool();
        setup_memory_pool();
        setup_descriptor_pool();
        setup_renderpasses();
        setup_framebuffers();
        setup_texture();
        setup_uniform_buffer();
        setup_vertex_buffer();
        setup_descriptor_sets();
        setup_shaders();
        setup_pipeline_cache();
        setup_pipeline_layouts();
        setup_graphics_pipelines();
        // setup_compute_pipelines();
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

    void setup_command_pool() {
        ev::logger::Logger::getInstance().info("[Setup Command Pool]");
        command_pool = std::make_shared<ev::CommandPool>(device, 
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT
        );

        for ( size_t i = 0 ; i < swapchain->get_images().size(); ++i ) {
            commands_buffers.scene.push_back(command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY));
            commands_buffers.outline.push_back(command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY));
            commands_buffers.composite.push_back(command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY));
        }
        ev::logger::Logger::getInstance().info("[Setup Command Pool Complete]");
    }

    void setup_memory_pool() {
        ev::logger::Logger::getInstance().info("[Setup Memory Pool]");
        memory_allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
        memory_allocator->add_pool(ev::memory_type::GPU_ONLY, 1024 * 1024 * 1024); // 1GB GPU 전용 메모리 풀
        memory_allocator->add_pool(ev::memory_type::HOST_READABLE, 256 * 1024 * 1024); // 256MB 호스트 읽기 가능한 메모리
        CHECK_RESULT(memory_allocator->build());
        ev::logger::Logger::getInstance().info("[Setup Memory Pool Complete]");
    }

    void setup_descriptor_pool() {
        ev::logger::Logger::getInstance().info("[Setup Descriptor Pool]");
        descriptor_pool = std::make_shared<ev::DescriptorPool>(device);
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 40);
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 40);
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 40);
        descriptor_pool->add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 40);
        descriptor_pool->create_pool(100, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
        ev::logger::Logger::getInstance().info("[Setup Descriptor Pool Complete]");
    }

    void setup_renderpasses() {
        VkFormat color_format = swapchain->get_image_format();
        VkFormat depth_stencil_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
        
        // Scene Color Attachment
        VkAttachmentDescription color_attachment = {};
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.format = color_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.flags = 0;

        // Scene Depth Attachment
        VkAttachmentDescription depth_attachment = {};
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.format = depth_stencil_format;
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.flags = 0;

        std::vector<VkAttachmentDescription> attachments = {color_attachment, depth_attachment};
        std::vector<VkSubpassDependency> subpass_dependencies;
        std::vector<VkSubpassDescription> subpasses;

        /** Scene Renderpass */
        {
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ev::logger::Logger::getInstance().info("[Setup Scene Render Pass]");
            VkAttachmentReference color_reference = {};
            color_reference.attachment = 0; // Color attachment index
            color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            VkAttachmentReference depth_reference = {};
            depth_reference.attachment = 1; // Depth attachment index
            depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_reference;
            subpass.pDepthStencilAttachment = &depth_reference;
            subpasses.push_back(subpass);

            std::vector<VkSubpassDependency> dependencies(2);
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = 0;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            dependencies[0].dependencyFlags = 0;
            dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].dstSubpass = 0;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dependencyFlags = 0;

            render_passes.scene = std::make_shared<ev::RenderPass>(
                device, 
                attachments, 
                subpasses,
                dependencies
            );
            attachments.clear();
            subpasses.clear();
            subpass_dependencies.clear();
            ev::logger::Logger::getInstance().info("[Setup Scene Render Pass Complete]");
        } 

        // Outline
        {
            ev::logger::Logger::getInstance().info("[Setup Outline Render Pass]");
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // scene render pass 결과
            depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.format = depth_stencil_format;
            depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

            attachments.push_back(color_attachment);
            attachments.push_back(depth_attachment);

            VkAttachmentReference color_reference = {
                .attachment = 0 ,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference depth_reference = {
                .attachment = 1,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            };

            VkSubpassDescription subpass = {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = &color_reference,
                .pDepthStencilAttachment = &depth_reference
            };

            subpasses.push_back(subpass);

            VkSubpassDependency dependency = {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = 0
            };

            render_passes.outline = std::make_shared<ev::RenderPass>(
                device, 
                attachments, 
                subpasses,
                subpass_dependencies
            );
            attachments.clear();
            subpasses.clear();
            subpass_dependencies.clear();
            ev::logger::Logger::getInstance().info("[Setup Outline Render Pass Complete]");
        }

        // Composite
        {
            ev::logger::Logger::getInstance().info("[Setup Composite Render Pass]");
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            attachments.push_back(color_attachment);

            VkAttachmentReference color_reference = {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkSubpassDescription subpass = {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = &color_reference,
            };
            subpasses.push_back(subpass);

            VkSubpassDependency dependency = {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dependencyFlags = 0
            };
            subpass_dependencies.push_back(dependency);
            render_passes.composite = std::make_shared<ev::RenderPass>(
                device, 
                attachments, 
                subpasses,
                subpass_dependencies
            );
            attachments.clear();
            subpasses.clear();
            subpass_dependencies.clear();
            ev::logger::Logger::getInstance().info("[Setup Composite Render Pass Complete]");
        }
    }

    void setup_framebuffers() {
        ev::logger::Logger::getInstance().info("[Setup Framebuffers Start]");
        framebuffers.scene.clear();
        framebuffers.outline.clear();
        framebuffers.composite.clear();
        framebuffer_attachments.clear();
        d_buffers.scene_texture.clear();
        d_buffers.outline_texture.clear();

        VkFormat color_format = swapchain->get_image_format();
        for (const auto& image_view : swapchain->get_image_views()) {
            /**
             * Scene render pass 용 Framebuffer
             */

            ev::logger::Logger::getInstance().debug("[Setup Framebuffers] Creating Scene Framebuffer");
            std::shared_ptr<ev::Image> color_image = std::make_shared<ev::Image>(
                device,
                VK_IMAGE_TYPE_2D,
                color_format,
                display.width,
                display.height,
                1, 1, 1,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            );

            memory_allocator->allocate_image(color_image, ev::memory_type::GPU_ONLY);
            std::shared_ptr<ev::ImageView> color_image_view = std::make_shared<ev::ImageView>(
                device, 
                color_image,
                VK_IMAGE_VIEW_TYPE_2D, 
                color_format
            );

            auto color_sampler = std::make_shared<ev::Sampler>(
                device,
                VK_FILTER_LINEAR,
                VK_FILTER_LINEAR,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            );

            d_buffers.scene_texture.push_back(std::make_shared<ev::Texture>(
                color_image, 
                color_image_view, 
                color_sampler
            ));

            std::shared_ptr<ev::Image> depth_stencil_image = std::make_shared<ev::Image>(
                device,
                VK_IMAGE_TYPE_2D,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                display.width,
                display.height,
                1, 1, 1,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            );
            memory_allocator->allocate_image(depth_stencil_image, ev::memory_type::GPU_ONLY);
            VkComponentMapping component_mapping = ev::initializer::component_mapping();
            VkImageSubresourceRange subresource_range = ev::initializer::image_subresource_range(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
            std::shared_ptr<ev::ImageView> depth_stencil_view = std::make_shared<ev::ImageView>(
                device, 
                depth_stencil_image,
                VK_IMAGE_VIEW_TYPE_2D, 
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                component_mapping,
                subresource_range
            );

            std::vector<VkImageView> scene_attachments = {*color_image_view, *depth_stencil_view};
            framebuffers.scene.push_back(std::make_shared<ev::Framebuffer>(
                device, 
                render_passes.scene,
                scene_attachments,
                display.width,
                display.height,
                1
            ));

            framebuffer_attachments.push_back({color_image, color_image_view});
            ev::logger::Logger::getInstance().debug("[Setup Framebuffers] Scene Framebuffer Created");
            /**
             * Outline Framebuffer 생성
             */

            ev::logger::Logger::getInstance().debug("[Setup Framebuffers] Creating Outline Framebuffer");
            std::shared_ptr<ev::Image> outline_image = std::make_shared<ev::Image>(
                device,
                VK_IMAGE_TYPE_2D,
                color_format,
                display.width,
                display.height,
                1, 1, 1,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            );
            memory_allocator->allocate_image(outline_image, ev::memory_type::GPU_ONLY);
            std::shared_ptr<ev::ImageView> outline_image_view = std::make_shared
                <ev::ImageView>(
                device, 
                outline_image,
                VK_IMAGE_VIEW_TYPE_2D, 
                color_format
            );  

            auto outline_sampler = std::make_shared<ev::Sampler>(
                device,
                VK_FILTER_LINEAR,
                VK_FILTER_LINEAR,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            );

            d_buffers.outline_texture.push_back(std::make_shared<ev::Texture>(
                outline_image, 
                outline_image_view, 
                outline_sampler
            ));

            std::vector<std::shared_ptr<ev::ImageView>> outline_attachments = {outline_image_view, depth_stencil_view};
            framebuffers.outline.push_back(std::make_shared<ev::Framebuffer>(
                device, 
                render_passes.outline,
                outline_attachments,
                display.width,
                display.height,
                1
            ));
            ev::logger::Logger::getInstance().debug("[Setup Framebuffers] Outline framebuffer Created");

            /**
             * Composite Framebuffer 생성 
             */
            ev::logger::Logger::getInstance().debug("[Setup Framebuffers] Creating Composite Framebuffer");
            vector<VkImageView> attachments = {image_view};
            std::shared_ptr<ev::Framebuffer> framebuffer = std::make_shared<ev::Framebuffer>(
                device, 
                render_passes.composite,
                attachments,
                display.width,
                display.height,
                1
            );

            framebuffers.composite.push_back(framebuffer);

            queue->wait_idle(UINT32_MAX);
        }
        ev::logger::Logger::getInstance().info("[Setup Framebuffers End]");
    }

    void setup_descriptor_sets() {
        ev::logger::Logger::getInstance().info("[Setup Descriptor Sets Start]");

        descriptors.scene_layout = std::make_shared<ev::DescriptorSetLayout>(device);
        descriptors.scene_layout->add_binding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
        descriptors.scene_layout->create_layout();

        // descriptors.outline_layout = std::make_shared<ev::DescriptorSetLayout>(device);
        // descriptors.outline_layout->create_layout();

        descriptors.composite_layout = std::make_shared<ev::DescriptorSetLayout>(device);
        descriptors.composite_layout->add_binding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
        descriptors.composite_layout->add_binding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
        descriptors.composite_layout->create_layout();
        ev::logger::Logger::getInstance().info("[Setup Descriptor Set Layouts Complete]");

      
        ev::logger::Logger::getInstance().info("[Setup Composite Descriptor Set]");
        for ( size_t i = 0 ; i < swapchain->get_images().size() ; ++i ) {
            ev::logger::Logger::getInstance().info("[Write cube texture descriptor set]");
            descriptors.scene.push_back( descriptor_pool->allocate(descriptors.scene_layout) );
            descriptors.scene.back()->write_texture(
                0,
                d_buffers.texture,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            descriptors.scene.back()->update();
            ev::logger::Logger::getInstance().info("[Write cube texture descriptor set Complete]");


            ev::logger::Logger::getInstance().info("[Write composite texture descriptor set]");
            descriptors.composite.push_back( descriptor_pool->allocate(descriptors.composite_layout) );
            // d_buffers.scene_texture[i]->image->transient_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            // d_buffers.outline_texture[i]->image->transient_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            descriptors.composite.back()->write_texture(
                0,
                d_buffers.scene_texture[i],
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            descriptors.composite.back()->write_texture(
                1,
                d_buffers.outline_texture[i],
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            descriptors.composite.back()->update();
            ev::logger::Logger::getInstance().info("[Write composite texture descriptor set Complete]");
                        // descriptors.outline.push_back( descriptor_pool->allocate(descriptors.outline_layout) );
        }

        ev::logger::Logger::getInstance().info("[Setup Descriptor Sets End]");
    }

    void setup_shaders() {
        ev::logger::Logger::getInstance().info("[Setup Shaders]");
        auto scene_vert_path = shader_path / title / "scene.vert.spv";
        auto scene_frag_path = shader_path / title / "scene.frag.spv";
        auto outline_vert_path = shader_path / title / "outline.vert.spv";
        auto outline_frag_path = shader_path / title / "outline.frag.spv";
        auto composite_vert_path = shader_path / title / "composite.vert.spv";
        auto composite_frag_path = shader_path / title / "composite.frag.spv";
        auto blur_comp_path = shader_path / title / "blur.comp.spv";

        std::vector<uint32_t> scene_vert_code;
        ev::utility::read_spirv_shader_file(scene_vert_path.c_str(), scene_vert_code);
        std::vector<uint32_t> scene_frag_code;
        ev::utility::read_spirv_shader_file(scene_frag_path.c_str(), scene_frag_code);
        std::vector<uint32_t> outline_vert_code;
        ev::utility::read_spirv_shader_file(outline_vert_path.c_str(), outline_vert_code);
        std::vector<uint32_t> outline_frag_code;
        ev::utility::read_spirv_shader_file(outline_frag_path.c_str(), outline_frag_code);
        std::vector<uint32_t> composite_vert_code;
        ev::utility::read_spirv_shader_file(composite_vert_path.c_str(), composite_vert_code);
        std::vector<uint32_t> composite_frag_code;
        ev::utility::read_spirv_shader_file(composite_frag_path.c_str(), composite_frag_code);

        shaders.scene_vert = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, scene_vert_code);
        shaders.scene_frag = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, scene_frag_code);
        shaders.outline_vert = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, outline_vert_code);
        shaders.outline_frag = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, outline_frag_code);
        shaders.composite_vert = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, composite_vert_code);
        shaders.composite_frag = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, composite_frag_code);
        // shaders.blur_comp = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_COMPUTE_BIT, blur_comp_code);
        ev::logger::Logger::getInstance().info("[Setup Shaders Complete]");
    }

    void setup_pipeline_cache() {
        ev::logger::Logger::getInstance().info("[Setup Pipeline Cache]");
        pipeline_cache = std::make_shared<ev::PipelineCache>(device);
        ev::logger::Logger::getInstance().info("[Setup Pipeline Cache Complete]");
    }

    void setup_pipeline_layouts() {
        ev::logger::Logger::getInstance().info("[Setup Pipeline Layouts Start]");
        pipelines.scene_layout = std::make_shared<ev::PipelineLayout>(device,
            std::vector<std::shared_ptr<ev::DescriptorSetLayout>>{descriptors.scene_layout},
            std::vector<VkPushConstantRange>{ 
                ev::initializer::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBuffer))
             }
        );

        pipelines.outline_layout = std::make_shared<ev::PipelineLayout>(device,
            std::vector<std::shared_ptr<ev::DescriptorSetLayout>>{},
            std::vector<VkPushConstantRange>{ev::initializer::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBuffer))} // Fragment shader push constant
        );

        pipelines.composite_layout = std::make_shared<ev::PipelineLayout>(device,
            std::vector<std::shared_ptr<ev::DescriptorSetLayout>>{descriptors.composite_layout}
        );
        ev::logger::Logger::getInstance().info("[Setup Pipeline Layouts End]");
    }

    void setup_graphics_pipelines() {
        ev::logger::Logger::getInstance().info("[Setup Graphics Pipelines Start]");

        VkStencilOpState stencil_op_state = ev::initializer::stencil_op_state();
        // Scene Pipeline
        {
            stencil_op_state.writeMask = 0xFF;
            stencil_op_state.reference = 0x01;
            stencil_op_state.compareOp = VK_COMPARE_OP_ALWAYS;
            stencil_op_state.failOp = VK_STENCIL_OP_KEEP;
            stencil_op_state.passOp = VK_STENCIL_OP_REPLACE;

            ev::GraphicsPipelineBluePrintManager blueprint_manager(device, render_passes.scene);
            vector<shared_ptr<ev::GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
                .add_shader_stage(shaders.scene_vert)
                .add_shader_stage(shaders.scene_frag)
                .set_vertex_input_state()
                .add_vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
                .add_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
                .add_vertex_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3)
                .add_vertex_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6)
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
                .set_depth_stencil_state(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE,
                    VK_FALSE,
                    stencil_op_state, // Stencil Op State
                    stencil_op_state
                )
                .set_pipeline_layout(this->pipelines.scene_layout)
                .end_blueprint()
                .create_pipelines(pipeline_cache);
            // graphics_pipeline = pipelines[0];
            this->pipelines.scene = pipelines[0];
        }

        // Outline Pipeline
        {
            stencil_op_state.writeMask = 0x00;
            stencil_op_state.reference = 0x01;
            stencil_op_state.compareMask = 0xFF;
            stencil_op_state.compareOp = VK_COMPARE_OP_NOT_EQUAL;
            stencil_op_state.failOp = VK_STENCIL_OP_KEEP;
            stencil_op_state.passOp = VK_STENCIL_OP_KEEP;

            ev::GraphicsPipelineBluePrintManager blueprint_manager(device, render_passes.outline);
            vector<shared_ptr<ev::GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
                .add_shader_stage(shaders.outline_vert)
                .add_shader_stage(shaders.outline_frag)
                .set_vertex_input_state()
                .add_vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
                .add_vertex_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
                .add_vertex_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3)
                .add_vertex_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6)
                .set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .set_rasterization_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .set_dynamic_state()
                .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
                .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
                .add_viewport(display.width, display.height, 0.0f, 1.0f)
                .add_scissor(0, 0, display.width, display.height)
                .set_multisample_state(VK_SAMPLE_COUNT_1_BIT)
                .set_color_blend_state()
                .add_color_blend_attachment_state(VK_FALSE)
                .set_depth_stencil_state(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE,
                    VK_TRUE, 
                    stencil_op_state, // Stencil Op State
                    stencil_op_state  // Stencil Op State
                )
                .set_pipeline_layout(this->pipelines.outline_layout)
                .end_blueprint()
                .create_pipelines(pipeline_cache);
            this->pipelines.outline = pipelines[0];
        }

        // Composite
        {
            stencil_op_state= ev::initializer::stencil_op_state();
            ev::GraphicsPipelineBluePrintManager blueprint_manager(device, render_passes.composite);
            vector<shared_ptr<ev::GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
                .add_shader_stage(shaders.composite_vert)
                .add_shader_stage(shaders.composite_frag)
                .set_vertex_input_state()
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
                .set_depth_stencil_state(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE)
                .set_pipeline_layout(this->pipelines.composite_layout)
                .end_blueprint()
                .create_pipelines(pipeline_cache);
            this->pipelines.composite = pipelines[0];
        }

        ev::logger::Logger::getInstance().info("[Setup Graphics Pipelines End]");
    }

    void setup_texture() {
        uint8_t* texture_data = nullptr;
        int width = 0,height = 0, channels = 0, comp_req=0;
        std::string texture_path = resource_path / "textures" / "cube" / "wood.png";

        texture_data = stbi_load(texture_path.c_str(), &width, &height, &channels, comp_req);

        if (!texture_data) {
            throw std::runtime_error("Failed to load texture image");
        }

        std::shared_ptr<ev::Image> image = std::make_shared<ev::Image>(
            device,
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8G8B8A8_SRGB,
            500, 500, 1
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
        staging_buffer->flush();
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
        d_buffers.texture->image->transient_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        staging_buffer.reset();
        stbi_image_free(texture_data);
    }

    void setup_uniform_buffer() {
        ev::logger::Logger::getInstance().info("[Setup Uniform Buffer Start]");
        d_buffers.uniform= std::make_shared<ev::Buffer>(
            device, 
            sizeof(UniformBuffer), 
            ev::buffer_type::UNIFORM_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.uniform, ev::memory_type::HOST_READABLE);
        ev::logger::Logger::getInstance().info("[Setup Uniform Buffer End]");
    }

    void setup_vertex_buffer() {
        ev::logger::Logger::getInstance().info("[Setup Vertex Buffers Start]");
        ev::logger::Logger::getInstance().debug("Setting up vertex buffer...");
        sync_objects.copy_complete_fence->reset();
        d_buffers.cube_vertices= std::make_shared<ev::Buffer>(
            device, 
            cube_vertices.size() * sizeof(Vertex), 
            ev::buffer_type::VERTEX_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.cube_vertices, ev::memory_type::GPU_ONLY);

        ev::logger::Logger::getInstance().debug("[TextureCubeExample::setup_vertex_buffer] Creating vertex buffer with size: " + std::to_string(sizeof(cube_vertices)) + " bytes with vk handle : " + std::to_string(reinterpret_cast<uintptr_t>(VkBuffer(*d_buffers.cube_vertices))));

        d_buffers.cube_indices = std::make_shared<ev::Buffer>(
            device, 
            cube_indices.size() * sizeof(uint32_t), 
            ev::buffer_type::INDEX_BUFFER
        );
        memory_allocator->allocate_buffer(d_buffers.cube_indices, ev::memory_type::GPU_ONLY);

        shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            cube_vertices.size() * sizeof(Vertex), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );

        memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE);
        staging_buffer->map();
        staging_buffer->write(cube_vertices.data(), cube_vertices.size() * sizeof(Vertex));
        staging_buffer->flush();
        staging_buffer->unmap();
        auto index_staging_buffer = std::make_shared<ev::Buffer>(
            device, 
            cube_indices.size() * sizeof(uint32_t), 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        memory_allocator->allocate_buffer(index_staging_buffer, ev::memory_type::HOST_READABLE);
        index_staging_buffer->map();
        index_staging_buffer->write(cube_indices.data(), cube_indices.size() * sizeof(uint32_t));
        index_staging_buffer->flush();
        index_staging_buffer->unmap();

        shared_ptr<ev::CommandBuffer> staging_command = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        staging_command->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        staging_command->copy_buffer(d_buffers.cube_vertices, staging_buffer, cube_vertices.size() * sizeof(Vertex));
        staging_command->copy_buffer(d_buffers.cube_indices, index_staging_buffer, cube_indices.size() * sizeof(uint32_t));
        staging_command->end();
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, sync_objects.copy_complete_fence, nullptr));
        sync_objects.copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        staging_buffer.reset();
        index_staging_buffer.reset();
        

        ev::logger::Logger::getInstance().debug("Vertex buffer created successfully.");
        ev::logger::Logger::getInstance().debug("Vertex buffer handle: " + std::to_string(reinterpret_cast<uintptr_t>(VkBuffer(*d_buffers.cube_vertices))));
        ev::logger::Logger::getInstance().info("[Setup Vertex Buffers End]");
    }

    void draw() {
        ev::logger::Logger::getInstance().debug("[Draw Start]");
        sync_objects.frame_fences[current_frame_index]->reset();
        commands_buffers.scene[current_frame_index]->reset();
        ev::logger::Logger::getInstance().debug("Recording scene command buffer...");
        CHECK_RESULT(commands_buffers.scene[current_frame_index]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT));
        commands_buffers.scene[current_frame_index]->pipeline_barrier(
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            {
                ImageMemoryBarrier(
                    d_buffers.scene_texture[current_frame_index]->image,
                    VK_ACCESS_NONE_KHR, 
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED, 
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                )
            },
            {},
            {}
        );
        commands_buffers.scene[current_frame_index]->begin_render_pass(render_passes.scene, 
            framebuffers.scene[current_frame_index], 
            { VkClearValue{ .color = { { 0.0f, 0.0f, 0.2f, 1.0f } } }, VkClearValue{ .depthStencil = { 1.0f, 0 } }}, 
            VK_SUBPASS_CONTENTS_INLINE);
        commands_buffers.scene[current_frame_index]->set_viewport(0.0f, 0.0f, static_cast<float>(display.width), static_cast<float>(display.height));
        commands_buffers.scene[current_frame_index]->set_scissor(0, 0, display.width, display.height);
        commands_buffers.scene[current_frame_index]->bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.scene_layout, {descriptors.scene[current_frame_index]});
        commands_buffers.scene[current_frame_index]->bind_graphics_pipeline(pipelines.scene);
        commands_buffers.scene[current_frame_index]->bind_push_constants( pipelines.scene_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, &ubo, sizeof(UniformBuffer) );
        commands_buffers.scene[current_frame_index]->bind_vertex_buffers(0, {d_buffers.cube_vertices}, {0});
        commands_buffers.scene[current_frame_index]->bind_index_buffers({d_buffers.cube_indices}, 0, VK_INDEX_TYPE_UINT32);
        commands_buffers.scene[current_frame_index]->draw_indexed(cube_indices.size(), 1, 0, 0, 0);
        commands_buffers.scene[current_frame_index]->end_render_pass();

        CHECK_RESULT(commands_buffers.scene[current_frame_index]->end());
        ev::logger::Logger::getInstance().debug("Scene command buffer recorded successfully.");

        commands_buffers.outline[current_frame_index]->reset();
        CHECK_RESULT(commands_buffers.outline[current_frame_index]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT));
        ev::logger::Logger::getInstance().debug("Recording outline command buffer...");
        commands_buffers.outline[current_frame_index]->begin_render_pass(render_passes.outline, 
            framebuffers.outline[current_frame_index], 
            { VkClearValue{ .color = { { 0.0f, 0.0f, 0.2f, 1.0f } } }, VkClearValue{ .depthStencil = { 1.0f, 0 } }}, 
            VK_SUBPASS_CONTENTS_INLINE);
        commands_buffers.outline[current_frame_index]->set_viewport(0.0f, 0.0f, static_cast<float>(display.width), static_cast<float>(display.height));
        commands_buffers.outline[current_frame_index]->set_scissor(0, 0, display.width, display.height);
        commands_buffers.outline[current_frame_index]->bind_graphics_pipeline(pipelines.outline);
        commands_buffers.outline[current_frame_index]->bind_push_constants( pipelines.outline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, &ubo, sizeof(UniformBuffer) );
        commands_buffers.outline[current_frame_index]->bind_vertex_buffers(0, {d_buffers.cube_vertices}, {0});
        commands_buffers.outline[current_frame_index]->bind_index_buffers({d_buffers.cube_indices}, 0, VK_INDEX_TYPE_UINT32);
        commands_buffers.outline[current_frame_index]->draw_indexed(cube_indices.size(), 1, 0, 0, 0);
        commands_buffers.outline[current_frame_index]->end_render_pass();
        CHECK_RESULT(commands_buffers.outline[current_frame_index]->end());
        ev::logger::Logger::getInstance().debug("Outline command buffer recorded successfully.");

        commands_buffers.composite[current_frame_index]->reset();
        ev::logger::Logger::getInstance().debug("Recording composite command buffer...");
        CHECK_RESULT(commands_buffers.composite[current_frame_index]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT));

        commands_buffers.composite[current_frame_index]->pipeline_barrier(
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            {
                ImageMemoryBarrier(
                    d_buffers.scene_texture[current_frame_index]->image,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                ),
                ImageMemoryBarrier(
                    d_buffers.outline_texture[current_frame_index]->image,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                )
            },
            {},
            {}
        );

        commands_buffers.composite[current_frame_index]->begin_render_pass(render_passes.composite, 
            framebuffers.composite[current_frame_index], 
            { VkClearValue{ .color = { { 0.0f, 0.0f, 0.2f, 1.0f } } }, VkClearValue{ .depthStencil = { 1.0f, 0 } }}, 
            VK_SUBPASS_CONTENTS_INLINE);
        commands_buffers.composite[current_frame_index]->set_viewport(0.0f, 0.0f, static_cast<float>(display.width), static_cast<float>(display.height));
        commands_buffers.composite[current_frame_index]->set_scissor(0, 0, display.width, display.height);
        commands_buffers.composite[current_frame_index]->bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composite_layout, {descriptors.composite[current_frame_index]});
        commands_buffers.composite[current_frame_index]->bind_graphics_pipeline(pipelines.composite);
        commands_buffers.composite[current_frame_index]->bind_vertex_buffers(0, {d_buffers.cube_vertices}, {0});
        commands_buffers.composite[current_frame_index]->bind_index_buffers({d_buffers.cube_indices}, 0, VK_INDEX_TYPE_UINT32);
        commands_buffers.composite[current_frame_index]->draw_indexed(cube_indices.size(), 1, 0, 0, 0);
        commands_buffers.composite[current_frame_index]->end_render_pass();
        CHECK_RESULT(commands_buffers.composite[current_frame_index]->end());
        ev::logger::Logger::getInstance().debug("Composite command buffer recorded successfully.");

        std::vector<shared_ptr<ev::CommandBuffer>> _command_buffers = {
            commands_buffers.scene[current_frame_index],
            commands_buffers.outline[current_frame_index],
            commands_buffers.composite[current_frame_index]
        };
        VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        queue->submits(_command_buffers,
            std::vector<std::shared_ptr<ev::Semaphore>>{sync_objects.present_complete_semaphore[current_frame_index]},
            std::vector<std::shared_ptr<ev::Semaphore>>{sync_objects.render_complete_semaphore[current_frame_index]},
            &wait_stage_mask,
            sync_objects.frame_fences[current_frame_index]
        );
        sync_objects.frame_fences[current_frame_index]->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);

        if ( first_draw && (current_frame_index == swapchain->get_images().size() - 1) ) {
            first_draw = false;
        }
        ev::logger::Logger::getInstance().debug("[Draw End]");
    }

    void uniform_update() {
        if ( last_frame_time == 0.0f ) {
            last_frame_time = current_frame_time = glfwGetTime();
        } else {
            current_frame_time = glfwGetTime();
        }
        ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        ubo.model = glm::rotate(ubo.model, glm::radians(current_frame_time * 50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.model = glm::scale(ubo.model, glm::vec3(0.5f, 0.5f, 0.5f)); // 크기 조정
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

        d_buffers.uniform->map();
        d_buffers.uniform->write(&ubo, sizeof(UniformBuffer));
        d_buffers.uniform->flush();
        d_buffers.uniform->unmap();
        last_frame_time = current_frame_time;
    }

    void render() override {
        // Triangle rendering code goes here
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

        uniform_update();
        draw();
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
     }



    void on_window_resize() {
        // Handle window resize
        // swapchain->create(swapchain->get_surface(), display.width, display.height);
        ev::logger::Logger::getInstance().debug("Window resized to: " + std::to_string(display.width) + "x" + std::to_string(display.height));
    }
};

RUN_EXAMPLE_MAIN(ExampleImpl, "outline", false)