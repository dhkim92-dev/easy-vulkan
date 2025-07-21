#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <thread>
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
        std::shared_ptr<ev::RenderPass> base;
    } render_passes;

    struct {
        std::vector<std::shared_ptr<ev::Framebuffer>> base;
    } framebuffers;

    struct CommandBuffers {
        std::vector<std::shared_ptr<ev::CommandBuffer>> base;
        std::vector<std::shared_ptr<ev::CommandBuffer>> blur;
    } commands_buffers;

    struct {
        std::vector<std::shared_ptr<ev::DescriptorSet>> base_origin; // base render pass descriptor set, original image
        std::vector<std::shared_ptr<ev::DescriptorSet>> base_blurred; // base render pass descriptor set, blurred image
        std::vector<std::shared_ptr<ev::DescriptorSet>> blur; // compute shader descriptor set
        std::shared_ptr<ev::DescriptorSetLayout> base_layout;
        std::shared_ptr<ev::DescriptorSetLayout> blur_layout;
    } descriptors;

    struct { 
        std::shared_ptr<ev::GraphicsPipeline> base;
        std::shared_ptr<ev::ComputePipeline> blur;
        std::shared_ptr<ev::PipelineLayout> base_layout;
        std::shared_ptr<ev::PipelineLayout> blur_layout;
    } pipelines;

    struct {
        std::shared_ptr<ev::Shader> blur_comp;
        std::shared_ptr<ev::Shader> base_vert;
        std::shared_ptr<ev::Shader> base_frag;
    } shaders;

    struct {
        std::shared_ptr<ev::Texture> input_texture;
        std::shared_ptr<ev::Texture> output_texture;
        std::shared_ptr<ev::Buffer> filter;
    } d_buffers;

    struct {
        std::vector<float> filter;
    } h_buffers;

    struct {
        std::vector<std::shared_ptr<ev::Semaphore>> compute_complete_semaphore;
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
        this->display.width = 660;
        this->display.height = 330;
        setup_default_context();
        setup_synchronize_objects();
        setup_queue();
        setup_command_pool();
        setup_memory_pool();
        setup_descriptor_pool();
        setup_renderpasses();
        setup_framebuffers();
        setup_texture();
        setup_descriptor_sets();
        setup_shaders();
        setup_pipeline_cache();
        setup_pipeline_layouts();
        setup_graphics_pipelines();
        setup_compute_pipelines();
    }

    void setup_synchronize_objects() {
        ev::logger::Logger::getInstance().info("[Setup Synchronization Objects Start]");
        for (uint32_t i = 0 ; i < swapchain->get_images().size(); ++i) {
            sync_objects.render_complete_semaphore.push_back(std::make_shared<ev::Semaphore>(device));
            sync_objects.present_complete_semaphore.push_back(std::make_shared<ev::Semaphore>(device));
            sync_objects.compute_complete_semaphore.push_back(std::make_shared<ev::Semaphore>(device));
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
            commands_buffers.base.push_back(command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY));
            commands_buffers.blur.push_back(command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY));
        }
        ev::logger::Logger::getInstance().info("[Setup Command Pool Complete]");
    }

    void setup_memory_pool() {
        ev::logger::Logger::getInstance().info("[Setup Memory Pool]");
        memory_allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
        memory_allocator->add_pool(ev::memory_type::GPU_ONLY, 128 * MB); // 128MB GPU 전용 메모리 풀
        memory_allocator->add_pool(ev::memory_type::HOST_READABLE, 8*MB); // 8MB 호스트 읽기 가능한 메모리
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
        ev::logger::Logger::getInstance().info("[Setup Render Passes Start]");
        VkFormat color_format = swapchain->get_image_format();
        VkFormat depth_stencil_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
        
        // Base Color Attachment
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

        std::vector<VkAttachmentDescription> attachments;// = {color_attachment};//, depth_attachment};
        std::vector<VkSubpassDependency> subpass_dependencies;
        std::vector<VkSubpassDescription> subpasses;
 
        // Base
        {
            ev::logger::Logger::getInstance().info("[Setup Base Render Pass]");
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
                .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dependencyFlags = 0
            };
            subpass_dependencies.push_back(dependency);
            render_passes.base = std::make_shared<ev::RenderPass>(
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
        framebuffers.base.clear();

        VkFormat color_format = swapchain->get_image_format();
        for (const auto& image_view : swapchain->get_image_views()) {
            /**
             * Composite Framebuffer 생성 
             */
            ev::logger::Logger::getInstance().debug("[Setup Framebuffers] Creating Composite Framebuffer");
            vector<VkImageView> attachments = {image_view};
            std::shared_ptr<ev::Framebuffer> framebuffer = std::make_shared<ev::Framebuffer>(
                device, 
                render_passes.base,
                attachments,
                display.width,
                display.height,
                1
            );

            framebuffers.base.push_back(framebuffer);
        }
        ev::logger::Logger::getInstance().info("[Setup Framebuffers End]");
    }

    void setup_descriptor_sets() {
        ev::logger::Logger::getInstance().info("[Setup Descriptor Sets Start]");

        descriptors.blur_layout = std::make_shared<ev::DescriptorSetLayout>(device);
        descriptors.blur_layout->add_binding(VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0); // input image
        descriptors.blur_layout->add_binding(VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1); // blurred image
        descriptors.blur_layout->create_layout();
        descriptors.base_layout = std::make_shared<ev::DescriptorSetLayout>(device); 
        descriptors.base_layout->add_binding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // target image to fill frame buffer
        descriptors.base_layout->create_layout();
        ev::logger::Logger::getInstance().info("[Setup Descriptor Set Layouts Complete]");

        ev::logger::Logger::getInstance().info("[Setup Base Descriptor Set]");
        for ( size_t i = 0 ; i < swapchain->get_images().size() ; ++i ) {
            descriptors.blur.push_back(descriptor_pool->allocate(descriptors.blur_layout));
            descriptors.base_origin.push_back(descriptor_pool->allocate(descriptors.base_layout));
            descriptors.base_blurred.push_back(descriptor_pool->allocate(descriptors.base_layout));

            ev::logger::Logger::getInstance().info("[Write Descriptor Set for Blur]");
            descriptors.blur.back()->write_texture(
                0,
                d_buffers.input_texture,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                VK_IMAGE_LAYOUT_GENERAL
            );

            descriptors.blur.back()->write_texture(
                1,
                d_buffers.output_texture,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                VK_IMAGE_LAYOUT_GENERAL
            );

            descriptors.blur.back()->update();
            ev::logger::Logger::getInstance().info("[Write Descriptor Set for Blur Complete]");
            ev::logger::Logger::getInstance().info("[Write Descriptor Set for Base Origin]");
            descriptors.base_origin.back()->write_texture(
                0,
                d_buffers.input_texture,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            descriptors.base_origin.back()->update();
            ev::logger::Logger::getInstance().info("[Write Descriptor Set for Base Origin Complete]");

            ev::logger::Logger::getInstance().info("[Write Descriptor Set for Base Blurred]");

            descriptors.base_blurred.back()->write_texture(
                0,
                d_buffers.output_texture,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            descriptors.base_blurred.back()->update();
            ev::logger::Logger::getInstance().info("[Write Descriptor Set for Base Blurred Complete]");
        }

        ev::logger::Logger::getInstance().info("[Setup Descriptor Sets End]");
    }

    void setup_shaders() {
        ev::logger::Logger::getInstance().info("[Setup Shaders]");
        auto blur_comp_path = shader_path / title / "blur.comp.spv";
        auto base_vert_path = shader_path / title / "base.vert.spv";
        auto base_frag_path = shader_path / title / "base.frag.spv";

        std::vector<uint32_t> blur_comp_code;
        std::vector<uint32_t> base_vert_code;
        std::vector<uint32_t> base_frag_code;

        ev::utility::read_spirv_shader_file(blur_comp_path.c_str(), blur_comp_code);
        ev::utility::read_spirv_shader_file(base_vert_path.c_str(), base_vert_code);
        ev::utility::read_spirv_shader_file(base_frag_path.c_str(), base_frag_code);

        shaders.base_vert = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, base_vert_code);
        shaders.base_frag = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, base_frag_code);
        shaders.blur_comp = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_COMPUTE_BIT, blur_comp_code);

        ev::logger::Logger::getInstance().info("[Setup Shaders Complete]");
    }

    void setup_pipeline_cache() {
        ev::logger::Logger::getInstance().info("[Setup Pipeline Cache]");
        pipeline_cache = std::make_shared<ev::PipelineCache>(device);
        ev::logger::Logger::getInstance().info("[Setup Pipeline Cache Complete]");
    }

    void setup_pipeline_layouts() {
        ev::logger::Logger::getInstance().info("[Setup Pipeline Layouts Start]");
        pipelines.base_layout = std::make_shared<ev::PipelineLayout>(device,
            std::vector<std::shared_ptr<ev::DescriptorSetLayout>>{descriptors.base_layout}
        );
        ev::logger::Logger::getInstance().info("[Setup Pipeline Layouts End]");
    }

    void setup_graphics_pipelines() {
        ev::logger::Logger::getInstance().info("[Setup Graphics Pipelines Start]");

        ev::GraphicsPipelineBluePrintManager blueprint_manager(device, render_passes.base);
        vector<shared_ptr<ev::GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
            .add_shader_stage(shaders.base_vert)
            .add_shader_stage(shaders.base_frag)
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
            .set_depth_stencil_state(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER, VK_FALSE)
            .set_pipeline_layout(this->pipelines.base_layout)
            .end_blueprint()
            .create_pipelines(pipeline_cache);
        this->pipelines.base = pipelines[0];

        ev::logger::Logger::getInstance().info("[Setup Graphics Pipelines End]");
    }

    void setup_compute_pipelines() {
        ev::logger::Logger::getInstance().info("[Setup Compute Pipelines Start]");
        pipelines.blur_layout = std::make_shared<ev::PipelineLayout>(device,
            std::vector<std::shared_ptr<ev::DescriptorSetLayout>>{descriptors.blur_layout}
        );
        pipelines.blur = std::make_shared<ev::ComputePipeline>(device);
        pipelines.blur->set_layout(pipelines.blur_layout);
        pipelines.blur->set_shader(shaders.blur_comp);
        CHECK_RESULT(pipelines.blur->create_pipeline(pipeline_cache));
        ev::logger::Logger::getInstance().info("[Setup Compute Pipelines End]");
    }

    void setup_texture() {
        uint8_t* texture_data = nullptr;
        int width = 0,height = 0, channels = 0, comp_req=0;
        std::string texture_path = resource_path / "photos" / "cat-330x330.png";

        texture_data = stbi_load(texture_path.c_str(), &width, &height, &channels, comp_req);

        if (!texture_data) {
            throw std::runtime_error("Failed to load texture image");
        }

        std::shared_ptr<ev::Image> image = std::make_shared<ev::Image>(
            device,
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM,
            width, height, 1,
            1, 1, 
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        );

        memory_allocator->allocate_image(image, ev::memory_type::GPU_ONLY);

        std::shared_ptr<ev::ImageView> image_view = std::make_shared<ev::ImageView>(
            device, 
            image, 
            VK_IMAGE_VIEW_TYPE_2D, 
            VK_FORMAT_R8G8B8A8_UNORM
        );

        std::shared_ptr<ev::Sampler> sampler = std::make_shared<ev::Sampler>(
            device, 
            VK_FILTER_LINEAR, 
            VK_FILTER_LINEAR, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
        );

        d_buffers.input_texture = std::make_shared<ev::Texture>(
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
            d_buffers.input_texture->image,
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

        staging_command->copy_buffer_to_image(d_buffers.input_texture->image, 
            staging_buffer,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            {region}
        );

        std::vector<ev::ImageMemoryBarrier> post_copy_barriers;
        post_copy_barriers.reserve(1);
        post_copy_barriers.emplace_back( ev::ImageMemoryBarrier(
            d_buffers.input_texture->image,
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

        std::shared_ptr<ev::Fence> copy_complete_fence = std::make_shared<ev::Fence>(device);
        copy_complete_fence->reset();
        CHECK_RESULT(queue->submit(staging_command, {}, {}, nullptr, copy_complete_fence, nullptr));

        copy_complete_fence->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);
        d_buffers.input_texture->image->transient_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        staging_buffer.reset();
        stbi_image_free(texture_data);

        std::shared_ptr<ev::Image> output_image = std::make_shared<ev::Image>(
            device,
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM,
            width, height, 1, 1, 1,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
        );

        memory_allocator->allocate_image(output_image, ev::memory_type::GPU_ONLY);
        std::shared_ptr<ev::ImageView> output_image_view = std::make_shared<ev::ImageView>(
            device, 
            output_image, 
            VK_IMAGE_VIEW_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM
        );

        std::shared_ptr<ev::Sampler> output_sampler = std::make_shared<ev::Sampler>(
            device, 
            VK_FILTER_LINEAR, 
            VK_FILTER_LINEAR, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
        );

        d_buffers.output_texture = std::make_shared<ev::Texture>(
            output_image, 
            output_image_view, 
            output_sampler
        );

        {
            auto command_buffer = command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            VkImageSubresourceRange subresource_range = ev::initializer::image_subresource_range();
            ev::ImageMemoryBarrier barrier(
                d_buffers.output_texture->image,
                VK_ACCESS_NONE_KHR, 
                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_GENERAL,
                0, 0, subresource_range
            );
            command_buffer->pipeline_barrier(
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                {barrier}, 
                {}, 
                {}
            );
            command_buffer->end();
            std::shared_ptr<ev::Fence> fence = std::make_shared<ev::Fence>(device);
            fence->reset();
            CHECK_RESULT(queue->submit(command_buffer, {}, {}, nullptr, fence, nullptr));
            fence->wait(UINT64_MAX);
            queue->wait_idle(UINT64_MAX);
            d_buffers.output_texture->image->transient_layout(VK_IMAGE_LAYOUT_GENERAL);
        }
        ev::logger::Logger::getInstance().info("[Setup Texture Complete]");
    }

    void draw() {
        ev::logger::Logger::getInstance().debug("[Draw Start]");
        sync_objects.frame_fences[current_frame_index]->reset();

        commands_buffers.blur[current_frame_index]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
        commands_buffers.blur[current_frame_index]->pipeline_barrier(
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
            {
                ev::ImageMemoryBarrier(
                    d_buffers.input_texture->image,
                    VK_ACCESS_NONE_KHR, 
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL,
                    0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                ),
                ev::ImageMemoryBarrier(
                    d_buffers.output_texture->image,
                    VK_ACCESS_NONE_KHR, 
                    VK_ACCESS_SHADER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL,
                    0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                )
            }, 
            {}, 
            {}
        );
        commands_buffers.blur[current_frame_index]->bind_compute_pipeline(pipelines.blur);
        commands_buffers.blur[current_frame_index]->bind_descriptor_sets(
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelines.blur_layout,
            {descriptors.blur[current_frame_index]}
        );
        commands_buffers.blur[current_frame_index]->dispatch(
            (display.width + 31) / 32, 
            (display.height + 31) / 32, 
            1
        );

        commands_buffers.blur[current_frame_index]->pipeline_barrier(
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            {
                ev::ImageMemoryBarrier(
                    d_buffers.input_texture->image,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                ),

                ev::ImageMemoryBarrier(
                    d_buffers.output_texture->image,
                    VK_ACCESS_SHADER_WRITE_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                )
            }, 
            {}, 
            {}
        );
        commands_buffers.blur[current_frame_index]->end();
        ev::logger::Logger::getInstance().debug("[Blur Command Buffer End]");
        ev::logger::Logger::getInstance().debug("[Blur Command Buffer Submit]");
        VkPipelineStageFlags before_compute_wait_stages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        CHECK_RESULT(queue->submit(commands_buffers.blur[current_frame_index],
            {sync_objects.present_complete_semaphore[current_frame_index]},
            {sync_objects.compute_complete_semaphore[current_frame_index]},
            &before_compute_wait_stages,nullptr,nullptr
        ));
        ev::logger::Logger::getInstance().debug("[Blur Command Buffer Submit End]");

        commands_buffers.base[current_frame_index]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
        commands_buffers.base[current_frame_index]->bind_descriptor_sets(
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelines.base_layout,
            {descriptors.base_origin[current_frame_index]}
        );
        commands_buffers.base[current_frame_index]->begin_render_pass(
            render_passes.base,
            framebuffers.base[current_frame_index],
            {{0.0f, 0.0f, 0.2f, 1.0f}}
         );

        commands_buffers.base[current_frame_index]->bind_graphics_pipeline(pipelines.base);
        auto region = d_buffers.input_texture->image->get_extent();
        commands_buffers.base[current_frame_index]->set_viewport(0, 0, region.width, region.height);
        commands_buffers.base[current_frame_index]->set_scissor(0, 0, region.width, region.height);
        commands_buffers.base[current_frame_index]->draw(3, 1, 0, 0);
        commands_buffers.base[current_frame_index]->bind_descriptor_sets(
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelines.base_layout,
            {descriptors.base_blurred[current_frame_index]}
        );
        commands_buffers.base[current_frame_index]->set_viewport(region.width, 0, region.width, region.height);
        commands_buffers.base[current_frame_index]->set_scissor(region.width, 0, region.width, region.height);
        commands_buffers.base[current_frame_index]->draw(3, 1, 0, 0);
        commands_buffers.base[current_frame_index]->end_render_pass();

        commands_buffers.base[current_frame_index]->pipeline_barrier(
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
            {
                ev::ImageMemoryBarrier(
                    d_buffers.input_texture->image,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                )
            }, 
            {}, 
            {}
        );
        commands_buffers.base[current_frame_index]->pipeline_barrier(
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
            {
                ev::ImageMemoryBarrier(
                    d_buffers.output_texture->image,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                )
            }, 
            {}, 
            {}
        );
        commands_buffers.base[current_frame_index]->end();
        ev::logger::Logger::getInstance().debug("[Draw Base Blurred Command Buffer End]");
        ev::logger::Logger::getInstance().debug("[Draw Base Blurred Command Buffer Submit]");
        VkPipelineStageFlags compute_wait_stages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        CHECK_RESULT(queue->submit(commands_buffers.base[current_frame_index],
            {sync_objects.compute_complete_semaphore[current_frame_index]},
            {sync_objects.render_complete_semaphore[current_frame_index]},
            &compute_wait_stages, sync_objects.frame_fences[current_frame_index], nullptr
        ));
        ev::logger::Logger::getInstance().debug("[Draw Base Blurred Command Buffer Submit End]");
        sync_objects.frame_fences[current_frame_index]->wait(UINT64_MAX);
        queue->wait_idle(UINT64_MAX);

        ev::logger::Logger::getInstance().debug("[Draw End]");
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

        draw();
        result = queue->present(swapchain, current_frame_index, {
            sync_objects.render_complete_semaphore[current_frame_index]
        });
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

RUN_EXAMPLE_MAIN(ExampleImpl, "gaussian-blur", false)