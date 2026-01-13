#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "base/example_base.hpp"

class ExampleImpl : public ExampleBase {

private:

    vector<std::shared_ptr<ev::PipelineLayout>> pipeline_layouts;

    std::shared_ptr<ev::GraphicsPipeline> graphics_pipeline;

    std::shared_ptr<ev::PipelineCache> pipeline_cache;

    std::shared_ptr<ev::tools::gltf::Model> gltf_model;

    std::shared_ptr<ev::Buffer> camera_uniform;

    struct {
        std::shared_ptr<ev::DescriptorPool> pool;
        std::vector<std::shared_ptr<ev::DescriptorSetLayout>> layouts;
        std::vector<std::shared_ptr<ev::DescriptorSet>> sets;
        // std::vector<std::shared_ptr<ev::DescriptorSet>> texture_sets;
    } descriptors;

    struct {
        std::shared_ptr<ev::Shader> vertex;
        std::shared_ptr<ev::Shader> fragment;
    } shaders;

    struct Camera {
        glm::mat4 view;
        glm::mat4 proj;
    } camera;

    uint32_t current_frame_index = 0;

    float current_frame_time = 0.0f;

    float last_frame_time = 0.0f;

    glm::mat4 model_matrix = glm::mat4(1.0f);

public:

    explicit ExampleImpl(std::string example_name, std::string executable_path, bool debug = true)
    : ExampleBase(example_name, executable_path, debug) {
        this->title = example_name;
        setup_default_context();
        setup_descriptor_pool();
        setup_descriptor_set_layouts();
        setup_descriptor_set();
        setup_camera();
        setup_pipeline_layouts();
        setup_shaders();
        setup_model();
        setup_graphics_pipeline();
    }

    void setup_descriptor_pool() {
        ev_log_debug("[Setup Descriptor Sets Start]");
        descriptors.pool = std::make_shared<ev::DescriptorPool>(device);
        descriptors.pool->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100);
        descriptors.pool->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100);
        CHECK_RESULT(descriptors.pool->create_pool(6, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT));
        ev_log_debug("[Setup Descriptor Sets End]");
    }

    void setup_descriptor_set_layouts() {
        ev_log_debug("[Setup Descriptor Set Layouts Start]");
        std::shared_ptr<ev::DescriptorSetLayout> set1 = std::make_shared<ev::DescriptorSetLayout>(device);
        set1->add_binding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
        set1->create_layout();
        descriptors.layouts.push_back(set1);
        std::shared_ptr<ev::DescriptorSetLayout> set2 = std::make_shared<ev::DescriptorSetLayout>(device);
        set2->add_binding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
        set2->create_layout();
        descriptors.layouts.push_back(set2);
        ev_log_debug("[Setup Descriptor Set Layouts End]");
    }

    void setup_descriptor_set() {
        ev_log_debug("[Setup Descriptor Sets Start]");
        descriptors.sets.resize(swapchain->get_images().size());
        for (size_t i = 0; i < descriptors.sets.size(); ++i) {
            descriptors.sets[i] = descriptors.pool->allocate(descriptors.layouts[0]);
            if (!descriptors.sets[i]) {
                ev_log_error("Failed to allocate descriptor set");
                return;
            }
        }
        ev_log_debug("[Setup Descriptor Sets End]");
    }

    void setup_pipeline_layouts() {
        ev_log_debug("[Setup Pipeline Layouts Start]");
        VkPushConstantRange push_constant_range = {};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(glm::mat4);

        std::shared_ptr<ev::PipelineLayout> pipeline_layout 
        = std::make_shared<ev::PipelineLayout>(
            device, 
            descriptors.layouts,
            std::vector<VkPushConstantRange>{push_constant_range}
        );
        pipeline_layouts.emplace_back(pipeline_layout);
        ev_log_debug("[Setup Pipeline Layouts End]");
    }

    void setup_model() {
        std::string model_path = (resource_path / "glTF-Sample-Models" / "2.0" / "Cube" / "glTF" / "Cube.gltf").string();
        ev::tools::gltf::GLTFModelManager model_manager(
            device, 
            memory_allocator, 
            descriptors.pool,
            command_pool, 
            queue
            // model_path
        );
        model_manager.set_descriptor_binding_flags(
            ev::tools::gltf::DescriptorBindingFlags::ImageBaseColor
        );

        gltf_model = model_manager.load_model(model_path);
    }

    void create_memory_pool() override {
        ev_log_info("[Setup Memory Pool Start]");
        memory_allocator = std::make_shared<ev::BitmapBuddyMemoryAllocator>(device);
        memory_allocator->add_pool(ev::memory_type::GPU_ONLY, 512*MB); 
        memory_allocator->add_pool(ev::memory_type::HOST_READABLE, 128*MB);
        CHECK_RESULT(memory_allocator->build());
        ev_log_info("[Setup Memory Pool End]");
    }

    void setup_camera() {
        ev_log_debug("[Setup Camera Start]");
        camera.view = glm::lookAt(
            glm::vec3(0.0f, 2.0f, 5.0f), // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f), // Look at point
            glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
        );
        camera.proj = glm::perspective(glm::radians(45.0f), 
            static_cast<float>(display.width) / static_cast<float>(display.height), 
            0.1f, 
            100.0f);
        camera.proj[1][1] *= -1; // Invert Y axis for Vulkan


        camera_uniform = std::make_shared<ev::Buffer>(
            device, 
            sizeof(Camera), 
            ev::buffer_type::UNIFORM_BUFFER
        );
        memory_allocator->allocate_buffer(
            camera_uniform, 
            ev::memory_type::HOST_READABLE
        );
        camera_uniform->map();
        camera_uniform->write((void*)&camera, sizeof(Camera));
        camera_uniform->flush();
        camera_uniform->unmap();

        for ( size_t i = 0 ; i < swapchain->get_images().size(); ++i ) {
            descriptors.sets[i]->write_buffer(
                0,
                camera_uniform,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            );
            CHECK_RESULT(descriptors.sets[i]->update());
        }

        ev_log_debug("[Setup Camera End]");
    }

    void record_command_buffers() {
        ev_log_debug("[Record Command Buffers Start]");
        // sync_objects.frame_fences[current_buffer_index]->reset();
        command_buffers[current_buffer_index]->reset();
        CHECK_RESULT(command_buffers[current_buffer_index]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT));
        command_buffers[current_buffer_index]->begin_render_pass(render_pass, 
            framebuffers[current_buffer_index], 
            { VkClearValue{ .color = { { 0.0f, 0.0f, 0.2f, 1.0f } } }, VkClearValue{ .depthStencil = { 1.0f, 0 } }}, 
            VK_SUBPASS_CONTENTS_INLINE);
        command_buffers[current_buffer_index]->set_viewport(0.0f, 0.0f, static_cast<float>(display.width), static_cast<float>(display.height));
        command_buffers[current_buffer_index]->set_scissor(0, 0, display.width, display.height);
        command_buffers[current_buffer_index]->bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts[0], {descriptors.sets[current_buffer_index]}, 0);
        command_buffers[current_buffer_index]->bind_graphics_pipeline(graphics_pipeline);
        command_buffers[current_buffer_index]->bind_push_constants(
            pipeline_layouts[0], 
            VK_SHADER_STAGE_VERTEX_BIT, 
            0, 
            &model_matrix,
            sizeof(glm::mat4)
        );
        gltf_model->draw(command_buffers[current_buffer_index],
            ev::tools::gltf::RenderFlag::OPAQUE | ev::tools::gltf::RenderFlag::BIND_IMAGE,
            pipeline_layouts[0],
            1
        );

        command_buffers[current_buffer_index]->end_render_pass();
        CHECK_RESULT(command_buffers[current_buffer_index]->end());
    }

    void setup_shaders() {
        vector<uint32_t> vertex_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "mesh.vert.spv").string().c_str(), vertex_shader_code);
        shaders.vertex = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_code);
        vector<uint32_t> fragment_shader_code;
        ev::utility::read_spirv_shader_file( (shader_path / this->title / "mesh.frag.spv").string().c_str(), fragment_shader_code);
        shaders.fragment = std::make_shared<ev::Shader>(device, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_code);
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
        VkPipelineVertexInputStateCreateInfo* info = 
            ev::tools::gltf::Vertex::get_pipeline_vertex_input_state(
                { 
                    ev::tools::gltf::VertexType::Position,
                    ev::tools::gltf::VertexType::Normal,
                    ev::tools::gltf::VertexType::UV
                }
            );
        if (!info) {
            ev_log_error("Failed to get vertex input state");
            exit(EXIT_FAILURE);
        }
        ev::GraphicsPipelineBluePrintManager blueprint_manager(device, render_pass);
        vector<shared_ptr<ev::GraphicsPipeline>> pipelines = blueprint_manager.begin_blueprint()
            .add_shader_stage(shaders.vertex)
            .add_shader_stage(shaders.fragment)
            .set_vertex_input_state(info)
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
            // last_frame_time = current_frame_time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            last_frame_time = current_frame_time = static_cast<float>(glfwGetTime());
        } else {
            // current_frame_time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            current_frame_time = static_cast<float>(glfwGetTime());
        }

        model_matrix = glm::scale( glm::rotate(glm::mat4(1.0f), glm::radians(current_frame_time * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.5f, 0.5f, 0.5f));

        last_frame_time = current_frame_time;
    }

    void render() override {
        prepare_frame();
        record_command_buffers();
        uniform_update();
        submit_frame();
    }
};

RUN_EXAMPLE_MAIN(ExampleImpl, "gltf-model-loading", false)
