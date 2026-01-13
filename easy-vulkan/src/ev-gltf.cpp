#include "tools/ev-gltf.h"
#include <assert.h>
#include <cstdlib>

using namespace ev::tools::gltf;

VkVertexInputBindingDescription Vertex::vertex_binding_description = {};
std::vector<VkVertexInputAttributeDescription> Vertex::vertex_attribute_descriptions = {};
VkPipelineVertexInputStateCreateInfo Vertex::vertex_input_state_create_info = {};


VkVertexInputBindingDescription Vertex::input_binding_description(uint32_t binding) {
    VkVertexInputBindingDescription description{};
    description.binding = binding;
    description.stride = sizeof(Vertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Vertex input rate is per vertex
    return description;
}

VkVertexInputAttributeDescription Vertex::input_attribute_description(
    uint32_t binding, 
    uint32_t location,
    VertexType type
) {
    VkVertexInputAttributeDescription description{};
    description.binding = binding;
    description.location = location;
    description.offset = 0;

    switch (type) {
        case Position:
            description.format = VK_FORMAT_R32G32B32_SFLOAT;
            description.offset = offsetof(Vertex, pos);
            break;
        case Normal:
            description.format = VK_FORMAT_R32G32B32_SFLOAT;
            description.offset = offsetof(Vertex, normal);
            break;
        case UV:
            description.format = VK_FORMAT_R32G32_SFLOAT;
            description.offset = offsetof(Vertex, uv);
            break;
        case Color:
            description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            description.offset = offsetof(Vertex, color);
            break;
        case Joint:
            description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            description.offset = offsetof(Vertex, joint);
            break;
        case Weight:
            description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            description.offset = offsetof(Vertex, weight);
            break;
        case Tangent:
            description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            description.offset = offsetof(Vertex, tangent);
            break;
        default: 
            description = {};
    }
    return description;
}

std::vector<VkVertexInputAttributeDescription> Vertex::input_attribute_descriptions(
    uint32_t binding, 
    std::vector<VertexType> types
) {
    std::vector<VkVertexInputAttributeDescription> descriptions;
    for (size_t i = 0; i < types.size(); ++i) {
        descriptions.push_back(Vertex::input_attribute_description(binding, static_cast<uint32_t>(i), types[i]));
    }
    return descriptions;
}
VkPipelineVertexInputStateCreateInfo* Vertex::get_pipeline_vertex_input_state(const std::vector<VertexType> types) {
    vertex_binding_description = Vertex::input_binding_description(0);
    Vertex::vertex_attribute_descriptions = Vertex::input_attribute_descriptions(0, types);
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.pNext = nullptr;
    vertex_input_state_create_info.flags = 0;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &Vertex::vertex_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::vertex_attribute_descriptions.size());
    vertex_input_state_create_info.pVertexAttributeDescriptions = Vertex::vertex_attribute_descriptions.data();
    ev_log_debug("[ev::tools::gltf::Vertex] Vertex input state created with %u attributes.", vertex_input_state_create_info.vertexAttributeDescriptionCount);
    return &vertex_input_state_create_info;
}

Node::Node(uint32_t index): index(index) {

}

void Primitive::set_dimensions(glm::vec3 min_pos, glm::vec3 max_pos) {
    this->dimensions.min = min_pos;
    this->dimensions.max = max_pos;
    this->dimensions.size = max_pos - min_pos;
    this->dimensions.center = (min_pos + max_pos) * 0.5f;
    this->dimensions.radius = glm::distance(min_pos, max_pos) * 0.5f;
}

void Model::bind_buffers(std::shared_ptr<ev::CommandBuffer> command_buffer) {
    ev_log_debug("[ev::tools::gltf::Model] Binding vertex and index buffers");
    if (vertex_buffer) {
        command_buffer->bind_vertex_buffers(
            0,
            {this->vertex_buffer},
            {0}
        );
        ev_log_debug("[ev::tools::gltf::Model] Vertex buffer bound successfully.");
    }
    if (index_buffer) {
        command_buffer->bind_index_buffers(
            {this->index_buffer},
            0,
            VK_INDEX_TYPE_UINT32
        );
        ev_log_debug("[ev::tools::gltf::Model] Index buffer bound successfully.");
    }
    // buffer_bound = true;
}

void Model::draw_node(
    std::shared_ptr<ev::CommandBuffer> command_buffer,
    const std::shared_ptr<Node> node,
    uint32_t render_flags,
    std::shared_ptr<ev::PipelineLayout> pipeline_layout,
    uint32_t bind_image_set
) {
    if (!node->get_mesh()) {
        return; // No mesh to draw
    }

    for ( std::shared_ptr<Primitive> primitive : node->get_mesh()->get_primitives() ) {
        bool skip = false;
        auto material = primitive->get_material();
        if ( render_flags & RenderFlag::OPAQUE ) {
            skip =  material->get_alpha_mode() != Material::AlphaMode::OPAQUE;
        }
        if ( render_flags & RenderFlag::ALPHA_MASK ) {
            skip = material->get_alpha_mode() != Material::AlphaMode::MASK;
        }
        if ( render_flags & RenderFlag::ALPHA_BLEND ) {
            skip = material->get_alpha_mode() != Material::AlphaMode::BLEND;
        }
        if(skip) continue;

        if ( render_flags & RenderFlag::BIND_IMAGE ) {
            command_buffer->bind_descriptor_sets(
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_layout,
                {material->get_descriptor_set()},
                bind_image_set,
                {}
            );
        }

        command_buffer->draw_indexed(
            primitive->get_index_count(),
            1, primitive->get_first_index(),
            0, 0 // first instance
        );
    }

    for ( const auto& node : node->get_children() ) {
        draw_node(command_buffer, node, render_flags, pipeline_layout, bind_image_set);
    }
}

void Model::draw(std::shared_ptr<ev::CommandBuffer> command_buffer, 
    uint32_t render_flags, 
    std::shared_ptr<ev::PipelineLayout> pipeline_layout,
    uint32_t bind_image_set
) {
    bind_buffers(command_buffer);
    
    for (const auto& node : nodes) {
        draw_node(command_buffer, node, render_flags, pipeline_layout, bind_image_set);
    }
}

const std::shared_ptr<Node> Model::get_node_by_index(uint32_t index) const {
    for (const auto& node : nodes) {
        if (node->get_index() == index) {
            return node;
        }
    }
    return nullptr;
}

void Model::add_descriptor_set_layout(
    std::shared_ptr<ev::DescriptorSetLayout> descriptor_set_layout
) {
    descriptor_set_layouts.emplace_back(std::move(descriptor_set_layout));
}


GLTFModelManager::GLTFModelManager(
    std::shared_ptr<ev::Device> device,
    std::shared_ptr<ev::MemoryAllocator> memory_allocator,
    std::shared_ptr<ev::DescriptorPool> descriptor_pool,
    std::shared_ptr<ev::CommandPool> command_pool,
    std::shared_ptr<ev::Queue> transfer_queue
    // std::filesystem::path resource_path
) : device(std::move(device)),
    descriptor_pool(std::move(descriptor_pool)),
    memory_allocator(std::move(memory_allocator)),
    command_pool(std::move(command_pool)),
    transfer_queue(std::move(transfer_queue)) {
    // resource_path(resource_path) {

    if (!this->device) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] Device is null");
        exit(EXIT_FAILURE);
    }
    if (!this->memory_allocator) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] MemoryAllocator is null");
        exit(EXIT_FAILURE); 
    }
    if (!this->descriptor_pool) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] DescriptorPool is null");
        exit(EXIT_FAILURE);
    }
    if (!this->command_pool) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] CommandPool is null");
        exit(EXIT_FAILURE);
    }
    if (!this->transfer_queue) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] TransferQueue is null");
        exit(EXIT_FAILURE);
    }
}

std::shared_ptr<ev::tools::gltf::Model> GLTFModelManager::load_model(const std::string file_path) {

    tinygltf::Model gltf_model;
    tinygltf::TinyGLTF ctx;

    bool file_loaded = ctx.LoadASCIIFromFile(&gltf_model, nullptr, nullptr, file_path);

    resource_path = std::filesystem::path(file_path).parent_path();
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Resource path set to: %s", resource_path.string().c_str());

    if (!file_loaded) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] Failed to load glTF model from file: %s", file_path.c_str());
        exit(EXIT_FAILURE);
    }
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Successfully loaded glTF model from file: %s", file_path.c_str());

    std::shared_ptr<ev::tools::gltf::Model> model = std::make_shared<ev::tools::gltf::Model>(
        device
    );

    std::vector<ev::tools::gltf::Vertex> h_vertices;
    std::vector<uint32_t> h_indices;

    load_textures(gltf_model, model);
    load_materials(gltf_model, model);
    load_nodes(gltf_model, model, h_vertices, h_indices);
    load_animations(gltf_model, model);
    load_skins(gltf_model, model);
    
    // TODO: Linear node update

    setup_vertex_buffer(model, h_vertices);
    setup_index_buffer(model, h_indices);

    prepare_material_descriptor_sets(model);
    prepare_node_descriptor_sets(model);

    return model;
}

void GLTFModelManager::load_animations(
    tinygltf::Model& gltf_model, 
    std::shared_ptr<ev::tools::gltf::Model> model
) {

    ev_log_info("[ev::tools::gltf::GLTFModelManager::load_animations] Loading animations...");
    for (const auto& anim : gltf_model.animations) {
        auto animation = std::make_shared<ev::tools::gltf::Animation>(anim.name);
        if ( anim.name.empty() ) {
            animation->set_name("Animation_" + std::to_string(model->get_animations().size()));
        }         
        model->add_animation(animation);

        // Animation Samplers
        for ( auto& samp : anim.samplers ) {
            ev::tools::gltf::Animation::AnimationSampler sampler;

            if ( samp.interpolation == "LINEAR" ) {
                sampler.method = Animation::AnimationSampler::InterpolationMethod::LINEAR;
            } else if ( samp.interpolation == "STEP" ) {
                sampler.method = Animation::AnimationSampler::InterpolationMethod::STEP;
            } else if ( samp.interpolation == "CUBICSPLINE" ) {
                sampler.method = Animation::AnimationSampler::InterpolationMethod::CUBICSPLINE;
            }  
            // Input time values 
            {
                const auto& accessor = gltf_model.accessors[samp.input];
                const auto& buffer_view = gltf_model.bufferViews[accessor.bufferView];
                const auto& buffer = gltf_model.buffers[buffer_view.buffer];

                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                std::vector<float> buf(accessor.count);
                memcpy(buf.data(), buffer.data.data() + accessor.byteOffset + buffer_view.byteOffset, accessor.count * sizeof(float));
                for ( size_t index = 0 ; index < accessor.count ; ++index ) {
                    sampler.input_times.push_back(buf[index]);
                }
                buf.clear();

                for ( auto input : sampler.input_times ) {
                    animation->set_start_time(input);
                    animation->set_end_time(input);
                }
            }

            // Sampler TRS 값 읽기 
            {
                const auto& accessor = gltf_model.accessors[samp.output];
                const auto& buffer_view = gltf_model.bufferViews[accessor.bufferView];
                const auto& buffer = gltf_model.buffers[buffer_view.buffer];

                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                switch ( accessor.type ) {
                    case TINYGLTF_TYPE_VEC3 : {
                        std::vector<glm::vec3> buf(accessor.count);
                        memcpy(buf.data(), buffer.data.data() + accessor.byteOffset + buffer_view.byteOffset, accessor.count * sizeof(glm::vec3));
                        for ( size_t index = 0 ; index < accessor.count ; ++index ) {
                            sampler.outputs.emplace_back(
                                glm::vec4(buf[index], 1.0f) // Assuming vec3 is position, set w to 1.0f
                            );
                        }
                        buf.clear();
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4 : {
                        std::vector<glm::vec4> buf(accessor.count);
                        memcpy(buf.data(), buffer.data.data() + accessor.byteOffset + buffer_view.byteOffset, accessor.count * sizeof(glm::vec4));
                        for ( size_t index = 0 ; index < accessor.count ; ++index ) {
                            sampler.outputs.emplace_back(buf[index]);
                        }
                        buf.clear();
                        break;
                    }
                    default: 
                        ev_log_error("[ev::tools::gltf::GLTFModelManager::load_animations] Unsupported accessor type: %d", accessor.type);
                        break;
                }
                animation->add_sampler(sampler);
            }

            // channels
            {
                for (const auto& chan : anim.channels) {
                    auto channel = Animation::AnimationChannel();

                    if ( chan.target_path == "translation" ) {
                        channel.path_type = Animation::AnimationChannel::PathType::TRANSLATION;
                    } else if ( chan.target_path == "rotation" ) {
                        channel.path_type = Animation::AnimationChannel::PathType::ROTATION;
                    } else if ( chan.target_path == "scale" ) {
                        channel.path_type = Animation::AnimationChannel::PathType::SCALE;
                    } else if (chan.target_path == "weights") {
                        ev_log_error("[ev::tools::gltf::GLTFModelManager::load_animations] Unsupported animation channel path: %s", chan.target_path.c_str());
                        continue;
                    }

                    channel.sampler_index = chan.sampler;
                    channel.target_node = model->get_node_by_index(chan.target_node);

                    if ( channel.target_node == nullptr ) {
                        continue;
                    }
                    animation->add_channel(channel);
                }
            }
            model->add_animation(animation);
        }
    }

    ev_log_info("[ev::tools::gltf::GLTFModelManager::load_animations] Finished loading animations.");
}

void GLTFModelManager::load_skins(
    tinygltf::Model &gltf_model, 
    std::shared_ptr<ev::tools::gltf::Model> model
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager::load_skins] Loading skins...");
    for (const auto& skin : gltf_model.skins) {
        auto new_skin = std::make_shared<ev::tools::gltf::Skin>(skin.name);

        if ( skin.skeleton > -1 ) {
            new_skin -> set_skeleton_root(model->get_node_by_index(skin.skeleton));
        }

        // Load joints
        for (const auto& joint : skin.joints) {
            auto node = model->get_node_by_index(joint);
            if (node) {
                new_skin->add_joint(node);
            } else {
                ev_log_error("[ev::tools::gltf::GLTFModelManager::load_skins] Joint node not found: %u", joint);
            }
        }

        // Load inverse bind matrices
        if (skin.inverseBindMatrices >= 0) {
            const auto& accessor = gltf_model.accessors[skin.inverseBindMatrices];
            const auto& buffer_view = gltf_model.bufferViews[accessor.bufferView];
            const auto& buffer = gltf_model.buffers[buffer_view.buffer];

            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

            std::vector<glm::mat4> ibm(accessor.count);
            memcpy(ibm.data(), buffer.data.data() + accessor.byteOffset + buffer_view.byteOffset, accessor.count * sizeof(glm::mat4));
            for (size_t i = 0; i < ibm.size(); ++i) {
                new_skin->add_inverse_bind_matrix(ibm[i]);
            }
        }
        model->add_skin(new_skin);
    }

    ev_log_info("[ev::tools::gltf::GLTFModelManager::load_skins] Finished loading skins.");
}

std::shared_ptr<ev::Texture> GLTFModelManager::load_texture(tinygltf::Image &image, std::string& file_path) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager::load_texture] Loading image file: %s", file_path.c_str());
    if (!std::filesystem::exists(file_path)) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager::load_texture] Image file does not exist: %s", file_path.c_str());
        exit(EXIT_FAILURE);
    }
    // Load the image data from the file using stb_image

    uint8_t* buffer;
    uint32_t buffer_size;
    bool delete_buffer = false;

    if ( image.component == 4 ) {
        buffer_size = image.width * image.height * 4; // RGBA
        buffer = new uint8_t[buffer_size];
        uint8_t *rgba = buffer;
        uint8_t *rgb = &image.image[0];

        for ( size_t i = 0 ; i < image.width * image.height ; ++i ) {
            for ( uint32_t ch = 0 ; ch < 3 ; ++ch ) {
                rgba[ch] = rgb[ch];
            }
            rgba+=4;
            rgb+=3;
        }
        delete_buffer = true;
    } else {
        buffer_size = static_cast<uint32_t>(image.image.size());
        buffer = &image.image[0];
    }

    uint32_t width = static_cast<uint32_t>(image.width);
    uint32_t height = static_cast<uint32_t>(image.height);
    uint32_t mip_levels = static_cast<uint32_t>(
        std::floor(std::log2(std::max(width, height))) + 1.0
    );
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; // Assuming RGBA format
    VkFormatProperties props = device->get_physical_device()
        ->get_format_properties(format);

    if ( !(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) 
         || !(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager::load_texture] Format not supported for blit or sampled image: %d", format);
        exit(EXIT_FAILURE);
    }

    std::shared_ptr<ev::Image> texture_image = std::make_shared<ev::Image>(
        device,
        VK_IMAGE_TYPE_2D,
        format,
        width,
        height,
        1,
        mip_levels,
        1,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    );
    ev_log_debug("[ev::tools::gltf::GLTFModelManager::load_texture] Texture image created with size: %ux%u, mip levels: %u", width, height, mip_levels);

    std::shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
        device,
        buffer_size,
        ev::buffer_type::STAGING_BUFFER
    );
    ev_log_debug("[ev::tools::gltf::GLTFModelManager::load_texture] Staging buffer created with size: %u", buffer_size);

    if ( memory_allocator->allocate_image(texture_image, ev::memory_type::GPU_ONLY ) != VK_SUCCESS ) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager::load_texture] Failed to allocate image memory for texture.");
        exit(EXIT_FAILURE);
    }
    ev_log_debug("[ev::tools::gltf::GLTFModelManager::load_texture] Image memory allocated for texture.");

    if ( memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE) != VK_SUCCESS ) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager::load_texture] Failed to allocate buffer memory for staging buffer.");
        exit(EXIT_FAILURE);
    }
    ev_log_debug("[ev::tools::gltf::GLTFModelManager::load_texture] Buffer memory allocated for staging buffer.");

    staging_buffer->map(buffer_size);
    staging_buffer->write(buffer, buffer_size);
    staging_buffer->flush();
    staging_buffer->unmap();
    ev_log_debug("[ev::tools::gltf::GLTFModelManager::load_texture] Staging buffer created with size: %u", buffer_size);

    std::shared_ptr<ev::CommandBuffer> command_buffer = command_pool->allocate();
    command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    command_buffer->pipeline_barrier(
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        {ev::ImageMemoryBarrier(
            texture_image,
            VK_ACCESS_NONE_KHR,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        )},{},{}
    );
    texture_image->transient_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    command_buffer->copy_buffer_to_image(
        texture_image,
        staging_buffer,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        {region}
    );
    texture_image->transient_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    command_buffer->pipeline_barrier(
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        {ev::ImageMemoryBarrier(
            texture_image,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        )},{},{}
    );
    command_buffer->end();
    this->transfer_queue->submit(
        command_buffer,
        {},
        {},
        VK_NULL_HANDLE
    );
    this->transfer_queue->wait_idle(UINT32_MAX);
    ev_log_debug("[ev::tools::gltf::GLTFModelManager::load_texture] Image transfer completed.");
    staging_buffer.reset();

    std::shared_ptr<ev::CommandBuffer> blit_command = command_pool->allocate();
    blit_command->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    for ( uint32_t i = 1 ;i < mip_levels ; ++i ) {
        VkImageBlit blit = {};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcOffsets[1] = { int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1 };

        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = { int32_t(width >> i), int32_t(height >> i), 1 };

        VkImageSubresourceRange mip_range = {};
        mip_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mip_range.baseMipLevel = i;
        mip_range.levelCount = 1;
        mip_range.layerCount = 1;
        texture_image->transient_layout(VK_IMAGE_LAYOUT_UNDEFINED);
        blit_command->pipeline_barrier(
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            {ev::ImageMemoryBarrier(
                texture_image,
                0,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                mip_range
            )},{},{}
        );

        blit_command->blit_image(
            texture_image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            texture_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            {blit},
            VK_FILTER_LINEAR
        );
        texture_image->transient_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        blit_command->pipeline_barrier(
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            {ev::ImageMemoryBarrier(
                texture_image,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                mip_range
            )},{},{}
        );
    }

    blit_command->pipeline_barrier(
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        {ev::ImageMemoryBarrier(
            texture_image,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, 1}
        )},{},{}
    );
    blit_command->end();
    texture_image->transient_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    this->transfer_queue->submit(
        blit_command,
        {},
        {},
        VK_NULL_HANDLE
    );
    this->transfer_queue->wait_idle(UINT32_MAX);

    ev_log_debug("[ev::tools::gltf::GLTFModelManager::load_texture] Mipmaps generated for texture image.");

    if ( delete_buffer ) {
        delete[] buffer;
    }

    VkBorderColor border_color = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    std::shared_ptr<ev::Sampler> sampler = std::make_shared<ev::Sampler>(
        device,
        VK_FILTER_LINEAR,
        VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        border_color,
        0.0f, // mip_lod_bias
        8.0f, // max_anisotropy
        false,
        false,
        VK_COMPARE_OP_ALWAYS,
        0.0f, // min_lod
        static_cast<float>(mip_levels)
    );

    if (!sampler) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] Failed to create sampler.");
        exit(EXIT_FAILURE);
    }

    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = mip_levels;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    VkComponentMapping component_mapping = {};
    component_mapping.r = VK_COMPONENT_SWIZZLE_R;
    component_mapping.g = VK_COMPONENT_SWIZZLE_G;
    component_mapping.b = VK_COMPONENT_SWIZZLE_B;
    component_mapping.a = VK_COMPONENT_SWIZZLE_A;

    std::shared_ptr<ev::ImageView> image_view = std::make_shared<ev::ImageView>(device,
        texture_image,
        VK_IMAGE_VIEW_TYPE_2D,
        texture_image->get_format(),
        component_mapping,
        subresource_range
    );

    if (!image_view) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] Failed to create image view.");
        exit(EXIT_FAILURE);
    }

    std::shared_ptr<ev::Texture> texture = std::make_shared<ev::Texture>(
        texture_image,
        image_view,
        sampler
    );

    if (!texture) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] Failed to create texture.");
        exit(EXIT_FAILURE);
    }
    return texture;
}

void GLTFModelManager::load_textures(tinygltf::Model& gltf_model, 
    std::shared_ptr<Model> model
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Loading textures...");  
    for (tinygltf::Image& gltf_image : gltf_model.images) {
        if (gltf_image.uri.empty() && gltf_image.bufferView < 0) {
            ev_log_warn("[ev::tools::gltf::GLTFModelManager] Image has no URI or buffer view, skipping.");
            continue;
        }

        std::string file_path = (resource_path / gltf_image.uri).string();
        std::shared_ptr<ev::Texture> texture = load_texture(gltf_image, file_path);
        // model->get_textures().emplace_back(texture);
        texture->index = static_cast<uint32_t>(model->get_textures().size());
        model->add_texture(texture);
    }

    ev_log_debug("[ev::tools::gltf::GLTFModelManager] Number of textures: %u", static_cast<uint32_t>(gltf_model.textures.size()));
}

std::shared_ptr<ev::Texture> GLTFModelManager::get_texture(
    std::shared_ptr<Model> model,
    uint32_t idx
) {
    if (idx >= model->get_textures().size()) {
        ev_log_error("[ev::tools::gltf::GLTFModelManager] Texture index out of bounds:  %u", idx);
        return nullptr;
    }

    return model->get_textures()[idx];
}

void GLTFModelManager::load_materials(
    tinygltf::Model& gltf_model, 
    std::shared_ptr<Model> model
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Loading materials...");

    for (tinygltf::Material& mat : gltf_model.materials) {
        std::shared_ptr<Material> material = std::make_shared<Material>(device);

        if ( mat.values.find("baseColorTexture") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Base color factor found in material: %s", mat.name.c_str());
            uint32_t texture_idx = mat.values["baseColorTexture"].TextureIndex();
            uint32_t idx = gltf_model.textures[texture_idx].source;
            material->set_base_color_texture(get_texture(model, idx));
        }

        if ( mat.values.find("metallicRoughnessTexture") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Metallic roughness texture found in material: %s", mat.name.c_str());
            uint32_t texture_idx = mat.values["metallicRoughnessTexture"].TextureIndex();
            uint32_t idx = gltf_model.textures[texture_idx].source;
            material->set_metallic_roughness_texture(get_texture(model, idx));
        }

        if ( mat.values.find("roughnessFactor") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Roughness factor found in material: %s", mat.name.c_str());
            material->set_roughness_factor(static_cast<float>(mat.values["roughnessFactor"].Factor()));
        }

        if ( mat.values.find("metallicFactor") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Metallic factor found in material: %s", mat.name.c_str());
            material->set_metallic_factor(static_cast<float>(mat.values["metallicFactor"].Factor()));
        }

        if ( mat.values.find("baseColorFactor") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Base color factor found in material: %s", mat.name.c_str());
            const auto& color = mat.values["baseColorFactor"].ColorFactor();
            material->set_base_color_factor(glm::vec4(
                static_cast<float>(color[0]),
                static_cast<float>(color[1]),
                static_cast<float>(color[2]),
                static_cast<float>(color[3])
            ));
        }

        if ( mat.values.find("normalTexture") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Normal texture found in material: %s", mat.name.c_str());
            uint32_t texture_idx = mat.values["normalTexture"].TextureIndex();
            uint32_t idx = gltf_model.textures[texture_idx].source;
            material->set_normal_texture(get_texture(model, idx));
        }
        
        if ( mat.values.find("emissiveTexture") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Emissive texture found in material: %s", mat.name.c_str());
            // Check if the material has an emissive texture
            uint32_t texture_idx = mat.values["emissiveTexture"].TextureIndex();
            uint32_t idx = gltf_model.textures[texture_idx].source;
            material->set_emissive_texture(get_texture(model, idx));
        }

        if ( mat.values.find("occlusionTexture") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Occlusion texture found in material: %s", mat.name.c_str());
            uint32_t texture_idx = mat.values["occlusionTexture"].TextureIndex();
            uint32_t idx = gltf_model.textures[texture_idx].source;
            material->set_occlusion_texture(get_texture(model, idx));
        }

        if ( mat.values.find("alphaMode") != mat.values.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Alpha mode found in material: %s", mat.name.c_str());
            tinygltf::Parameter param = mat.additionalValues["alphaMode"];
            std::string alpha_mode = param.string_value;
            if (alpha_mode == "OPAQUE") {
                material->set_alpha_mode(Material::OPAQUE);
            } else if (alpha_mode == "MASK") {
                material->set_alpha_mode(Material::MASK);
            } else if (alpha_mode == "BLEND") {
                material->set_alpha_mode(Material::BLEND);
            }
        }

        if ( mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end() ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager] Alpha cutoff found in material: %s", mat.name.c_str());
            // Check if
            material->set_alpha_cutoff(static_cast<float>(mat.additionalValues["alphaCutoff"].Factor()));
        }

        model->add_material(material);
    }
    model->add_material(std::make_shared<Material>(device)); // Default material
    ev_log_debug("[ev::tools::gltf::GLTFModelManager] Number of materials: %zu", gltf_model.materials.size());
}

void GLTFModelManager::load_nodes(
    tinygltf::Model& gltf_model, 
    std::shared_ptr<Model> model,
    std::vector<Vertex> &h_vertices,
    std::vector<uint32_t> &h_indices,
    float scale_factor
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Loading nodes...");

    const tinygltf::Scene& scene = gltf_model.scenes[gltf_model.defaultScene > -1 ? gltf_model.defaultScene : 0];
    for ( size_t i = 0 ; i < scene.nodes.size() ; ++i ) {
        uint32_t node_idx = scene.nodes[i];
        tinygltf::Node& node = gltf_model.nodes[node_idx];
        load_node(
            nullptr,
            gltf_model,
            model,
            node,
            node_idx,
            h_vertices,
            h_indices,
            scale_factor
        );
    }

    ev_log_debug("[ev::tools::gltf::GLTFModelManager] Number of nodes: %zu", gltf_model.nodes.size());
}

void GLTFModelManager::load_node(
    std::shared_ptr<ev::tools::gltf::Node> parent_node,
    tinygltf::Model& gltf_model,
    std::shared_ptr<Model> model,
    tinygltf::Node& node,
    uint32_t node_idx,
    std::vector<Vertex> &h_vertices,
    std::vector<uint32_t> &h_indices,
    float scale_factor
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Loading node: %s", node.name.c_str());

    std::shared_ptr<ev::tools::gltf::Node> new_node = std::make_shared<ev::tools::gltf::Node>(node_idx);
    new_node->set_name(node.name);
    // new_node->set_index(node_idx);
    new_node->set_parent(parent_node);
    new_node->set_skin_index(node.skin);

    if (parent_node) {
        parent_node->add_child(new_node);
    } else {
        new_node->set_parent(nullptr);
    }

    glm::vec3 translation = glm::vec3(0.0f);
    if (node.translation.size() == 3) {
        translation = glm::make_vec3(node.translation.data());
        new_node->set_translation(translation);
    }

    glm::mat4 rotation = glm::mat4(1.0f);
    if (node.rotation.size() == 4) {
        glm::quat quat = glm::make_quat(node.rotation.data());
        new_node->set_rotation(quat);
    }

    glm::vec3 scale = glm::vec3(1.0f);
    if (node.scale.size() == 3) {
        scale = glm::make_vec3(node.scale.data());
        new_node->set_scale(scale);
    }

    if (node.matrix.size() == 16) {
        auto _matrix = glm::make_mat4x4(node.matrix.data());
        new_node->set_matrix(_matrix);
    }

    if ( node.children.size() > 0 ) {
        for ( size_t i = 0 ; i < node.children.size() ; ++i ) {
            uint32_t child_idx = node.children[i];
            tinygltf::Node& child_node = gltf_model.nodes[child_idx];
            load_node(
                new_node,
                gltf_model,
                model,
                child_node,
                child_idx,
                h_vertices,
                h_indices,
                scale_factor
            );
        }
    }

    load_node_meshes(gltf_model, model, node, new_node, h_vertices, h_indices);
}

void GLTFModelManager::load_node_meshes(
    tinygltf::Model& gltf_model,
    std::shared_ptr<ev::tools::gltf::Model> model,
    tinygltf::Node& node,
    std::shared_ptr<ev::tools::gltf::Node> new_node,
    std::vector<Vertex> &h_vertices,
    std::vector<uint32_t> &h_indices
) {
    if (node.mesh < 0) {
        return; // No mesh associated with this node
    }

    tinygltf::Mesh& mesh = gltf_model.meshes[node.mesh];
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Loading mesh: %s", mesh.name.c_str());
    std::shared_ptr<ev::tools::gltf::Mesh> new_mesh
        = std::make_shared<ev::tools::gltf::Mesh>();
    new_mesh->set_name(mesh.name);
    new_mesh->set_uniform_buffer(
        std::make_shared<ev::Buffer>(
            device,
            sizeof(ev::tools::gltf::Mesh::Uniform),
            ev::buffer_type::UNIFORM_BUFFER
        )
    );
    memory_allocator->allocate_buffer(new_mesh->get_uniform_buffer(), 
        ev::memory_type::HOST_READABLE);
    new_mesh->get_uniform_data().matrix = new_node->get_matrix();

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
        const tinygltf::Primitive& primitive = mesh.primitives[i];

        if ( primitive.indices < 0 ) {
            continue;
        }

        glm::vec3 min_pos(0.0f);
        glm::vec3 max_pos(0.0f);

        uint32_t vertex_start = static_cast<uint32_t>(h_vertices.size());
        uint32_t vertex_count = 0;
        uint32_t index_start = static_cast<uint32_t>(h_indices.size());
        uint32_t index_count = 0;

        add_mesh_vertices(gltf_model, mesh, primitive, h_vertices, min_pos, max_pos, vertex_start, vertex_count);
        add_mesh_indices(gltf_model, 
            model, 
            mesh, 
            new_mesh, 
            primitive, 
            h_indices, 
            vertex_start, 
            vertex_count, 
            index_start, 
            index_count, 
            min_pos, 
            max_pos
        );
        new_node->set_mesh(new_mesh);
        auto parent = new_node->get_parent();
        if ( !parent.expired() ) {
            parent.lock()->add_child(new_node);
        } else {
            model->add_node(new_node);
        }
        model->add_linear_node(new_node);
    }
}

void GLTFModelManager::add_mesh_indices(
    tinygltf::Model& gltf_model,
    std::shared_ptr<ev::tools::gltf::Model> model,
    tinygltf::Mesh& mesh,
    std::shared_ptr<ev::tools::gltf::Mesh> new_mesh,
    const tinygltf::Primitive& primitive,
    std::vector<uint32_t> &h_indices,
    uint32_t& vertex_start,
    uint32_t& vertex_count,
    uint32_t& index_start,
    uint32_t& index_count,
    glm::vec3& min_pos,
    glm::vec3& max_pos
) {
    const tinygltf::Accessor accessor = gltf_model.accessors[primitive.indices];
    const tinygltf::BufferView& buffer_view = gltf_model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = gltf_model.buffers[buffer_view.buffer];

    index_count = static_cast<uint32_t>(accessor.count);

    switch(accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            std::vector<uint16_t> buf = std::vector<uint16_t>(accessor.count);
            memcpy(buf.data(), &buffer.data[accessor.byteOffset + buffer_view.byteOffset], index_count * sizeof(uint16_t));
            for ( size_t index = 0 ; index < accessor.count ; ++index ) {
                h_indices.emplace_back(buf[index] + vertex_start);
            }
            buf.clear();
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            std::vector<uint32_t> buf = std::vector<uint32_t>(accessor.count);
            memcpy(buf.data(), &buffer.data[accessor.byteOffset + buffer_view.byteOffset], index_count * sizeof(uint32_t));
            for ( size_t index = 0 ; index < accessor.count ; ++index ) {
                h_indices.emplace_back(buf[index] + vertex_start);
            }
            buf.clear();
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            std::vector<uint8_t> buf = std::vector<uint8_t>(accessor.count);
            memcpy(buf.data(), &buffer.data[accessor.byteOffset + buffer_view.byteOffset], index_count * sizeof(uint8_t));
            for ( size_t index = 0 ; index < accessor.count ; ++index ) {
                h_indices.emplace_back(static_cast<uint32_t>(buf[index]) + vertex_start);
            }
            buf.clear();
            break;
        }
        default:
            ev_log_error("[ev::tools::gltf::GLTFModelManager] Unsupported index component type.");
            return;
    }

    std::shared_ptr<ev::tools::gltf::Primitive> new_primitive =
        std::make_shared<ev::tools::gltf::Primitive>(
            index_start,
            index_count,
            vertex_start,
            vertex_count,
            primitive.material > -1 ? model->get_materials()[primitive.material] : model->get_materials().back()
        );
    new_primitive->set_dimensions(min_pos, max_pos);
    new_mesh->add_primitive(new_primitive);
}

void GLTFModelManager::add_mesh_vertices(
    tinygltf::Model& gltf_model,
    tinygltf::Mesh& mesh,
    const tinygltf::Primitive& primitive,
    std::vector<Vertex> &h_vertices,
    glm::vec3 &min_pos,
    glm::vec3 &max_pos,
    uint32_t &vertex_start,
    uint32_t &vertex_count
) {
    bool has_skin = false;
    vertex_start = static_cast<uint32_t>(h_vertices.size());
    vertex_count = 0;

    const float *buffer_pos = nullptr;
    const float *buffer_normal = nullptr;
    const float *buffer_uv = nullptr;
    const float *buffer_color = nullptr;
    const float *buffer_tangent = nullptr;
    const uint16_t *buffer_joint = nullptr;
    const float *buffer_weight = nullptr;
    uint32_t num_color_components;
    assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

    const tinygltf::Accessor& pos_accessor = gltf_model.accessors[primitive.attributes.find("POSITION")->second];
    const tinygltf::BufferView& pos_buffer_view = gltf_model.bufferViews[pos_accessor.bufferView];
    buffer_pos = reinterpret_cast<const float*>(& (gltf_model.buffers[pos_buffer_view.buffer].data[pos_accessor.byteOffset + pos_buffer_view.byteOffset]));
    min_pos = glm::vec3(pos_accessor.minValues[0], pos_accessor.minValues[1], pos_accessor.minValues[2]);
    max_pos = glm::vec3(pos_accessor.maxValues[0], pos_accessor.maxValues[1], pos_accessor.maxValues[2]);

    auto& attr = primitive.attributes;

    if ( attr.find("NORMAL") != attr.end() ) {
        const tinygltf::Accessor& normal_accessor = gltf_model.accessors[attr.find("NORMAL")->second];
        const tinygltf::BufferView& normal_buffer_view = gltf_model.bufferViews[normal_accessor.bufferView];
        buffer_normal = reinterpret_cast<const float*>(& (gltf_model.buffers[normal_buffer_view.buffer].data[normal_accessor.byteOffset + normal_buffer_view.byteOffset]));
    }
    
    if ( attr.find("TEXCOORD_0") != attr.end() ) {
        const tinygltf::Accessor& uv_accessor = gltf_model.accessors[attr.find("TEXCOORD_0")->second];
        const tinygltf::BufferView& uv_buffer_view = gltf_model.bufferViews[uv_accessor.bufferView];
        buffer_uv = reinterpret_cast<const float*>(& (gltf_model.buffers[uv_buffer_view.buffer].data[uv_accessor.byteOffset + uv_buffer_view.byteOffset]));
    }

    if ( attr.find("COLOR_0") != attr.end() ) {
        const tinygltf::Accessor& color_accessor = gltf_model.accessors[attr.find("COLOR_0")->second];
        const tinygltf::BufferView& color_buffer_view = gltf_model.bufferViews[color_accessor.bufferView];
        buffer_color = reinterpret_cast<const float*>(& (gltf_model.buffers[color_buffer_view.buffer].data[color_accessor.byteOffset + color_buffer_view.byteOffset]));
        num_color_components = color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ? 4 : 3;
    }

    if ( attr.find("TANGENT") != attr.end() ) {
        const tinygltf::Accessor& tangent_accessor = gltf_model.accessors[attr.find("TANGENT")->second];
        const tinygltf::BufferView& tangent_buffer_view = gltf_model.bufferViews[tangent_accessor.bufferView];
        buffer_tangent = reinterpret_cast<const float*>(& (gltf_model.buffers[tangent_buffer_view.buffer].data[tangent_accessor.byteOffset + tangent_buffer_view.byteOffset]));
    }

    if ( attr.find("JOINTS_0") != attr.end() ) {
        const tinygltf::Accessor& joint_accessor = gltf_model.accessors[attr.find("JOINTS_0")->second];
        const tinygltf::BufferView& joint_buffer_view = gltf_model.bufferViews[joint_accessor.bufferView];
        buffer_joint = reinterpret_cast<const uint16_t*>(& (gltf_model.buffers[joint_buffer_view.buffer].data[joint_accessor.byteOffset + joint_buffer_view.byteOffset]));
    }

    if (  attr.find("WEIGHTS_0") != attr.end() ) {
        const tinygltf::Accessor& weight_accessor = gltf_model.accessors[attr.find("WEIGHTS_0")->second];
        const tinygltf::BufferView& weight_buffer_view = gltf_model.bufferViews[weight_accessor.bufferView];
        buffer_weight = reinterpret_cast<const float*>(& (gltf_model.buffers[weight_buffer_view.buffer].data[weight_accessor.byteOffset + weight_buffer_view.byteOffset]));
    }

    has_skin = (buffer_joint != nullptr && buffer_weight != nullptr);
    vertex_count = static_cast<uint32_t>(pos_accessor.count);

    for ( size_t i = 0 ; i < pos_accessor.count ; ++i ) {
        Vertex vtx {};
        vtx.pos = glm::make_vec3(&buffer_pos[i*3]);
        vtx.normal = glm::normalize( glm::vec3( buffer_normal ? glm::make_vec3(&buffer_normal[i*3]) : glm::vec3(0.0f) ) );
        vtx.uv = buffer_uv ? glm::make_vec2(&buffer_uv[i*2]) : glm::vec2(0.0f);
        if ( buffer_color ) {
            switch(num_color_components) {
                case 3:
                    vtx.color = glm::vec4(glm::make_vec3(&buffer_color[i*3]), 1.0f);
                    break;
                case 4:
                    vtx.color = glm::make_vec4(&buffer_color[i*4]);
                    break;
            }
        } else {
            vtx.color = glm::vec4(1.0f);
        }
        vtx.tangent = buffer_tangent ? glm::make_vec4(&buffer_tangent[i*4]) : glm::vec4(0.0f);

        if ( has_skin ) {
            vtx.joint = glm::make_vec4(&buffer_joint[i*4]);
            vtx.weight = glm::make_vec4(&buffer_weight[i*4]);
        } else {
            vtx.joint = glm::vec4(0.0f);
            vtx.weight = glm::vec4(0.0f);
        }
        h_vertices.push_back(vtx);
    }
}

void GLTFModelManager::setup_vertex_buffer(
    std::shared_ptr<ev::tools::gltf::Model> model,
    std::vector<Vertex> &h_vertices
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Setting up vertex buffer...");

    std::shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
        device,
        h_vertices.size() * sizeof(Vertex),
        ev::buffer_type::STAGING_BUFFER
    );

    std::shared_ptr<ev::Buffer> vertex_buffer = std::make_shared<ev::Buffer>(
        device,
        h_vertices.size() * sizeof(Vertex),
        ev::buffer_type::VERTEX_BUFFER
    );

    memory_allocator->allocate_buffer(vertex_buffer, ev::memory_type::GPU_ONLY);
    memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE);
    staging_buffer->map(h_vertices.size() * sizeof(Vertex));
    staging_buffer->write(h_vertices.data(), h_vertices.size() * sizeof(Vertex));
    staging_buffer->flush();
    staging_buffer->unmap();

    std::shared_ptr<ev::CommandBuffer> command_buffer = command_pool->allocate();
    command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    command_buffer->copy_buffer(
        vertex_buffer,
        staging_buffer,
        h_vertices.size() * sizeof(Vertex)
    );
    command_buffer->end();

    std::shared_ptr<ev::Fence> fence = std::make_shared<ev::Fence>(device);
    fence->reset();

    transfer_queue->submit(
        command_buffer,
        {},
        {},
        nullptr,
        fence
    );
    fence->wait(UINT32_MAX);
    transfer_queue->wait_idle(UINT32_MAX);

    model->set_vertex_buffer(vertex_buffer);

    ev_log_debug("[ev::tools::gltf::GLTFModelManager] Vertex buffer setup complete.");
}

void GLTFModelManager::setup_index_buffer(
    std::shared_ptr<ev::tools::gltf::Model> model,
    std::vector<uint32_t> &h_indices
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager] Setting up index buffer...");

    std::shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
        device,
        h_indices.size() * sizeof(uint32_t),
        ev::buffer_type::STAGING_BUFFER
    );

    std::shared_ptr<ev::Buffer> index_buffer = std::make_shared<ev::Buffer>(
        device,
        h_indices.size() * sizeof(uint32_t),
        ev::buffer_type::INDEX_BUFFER
    );

    memory_allocator->allocate_buffer(index_buffer, ev::memory_type::GPU_ONLY);
    memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE);
    staging_buffer->map(h_indices.size() * sizeof(uint32_t));
    staging_buffer->write(h_indices.data(), h_indices.size() * sizeof(uint32_t));
    staging_buffer->flush();
    staging_buffer->unmap();

    std::shared_ptr<ev::CommandBuffer> command_buffer = command_pool->allocate();
    command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    command_buffer->copy_buffer(
        index_buffer,
        staging_buffer,
        h_indices.size() * sizeof(uint32_t)
    );
    command_buffer->end();

    std::shared_ptr<ev::Fence> fence = std::make_shared<ev::Fence>(device);
    fence->reset();
    transfer_queue->submit(
        command_buffer,
        {},
        {},
        nullptr,
        fence
    );
    fence->wait(UINT32_MAX);
    transfer_queue->wait_idle(UINT32_MAX);
    model->set_index_buffer(index_buffer);

    ev_log_debug("[ev::tools::gltf::GLTFModelManager] Index buffer setup complete.");
}

void GLTFModelManager::prepare_material_descriptor_sets(
    std::shared_ptr<ev::tools::gltf::Model> model
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager::prepare_material_descriptor_sets] Preparing material descriptor sets...");

    std::shared_ptr<ev::DescriptorSetLayout> texture_layout
        = std::make_shared<ev::DescriptorSetLayout>(device);

    if ( descriptor_binding_flags & DescriptorBindingFlags::ImageBaseColor ) {
        ev_log_debug("[ev::tools::gltf::GLTFModelManager::prepare_material_descriptor_sets] Adding base color image binding.");
        texture_layout->add_binding(
            VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            static_cast<uint32_t>(texture_layout->get_bindings().size())    ,
            1
        );
    }

    if ( descriptor_binding_flags & DescriptorBindingFlags::ImageNormalMap ) {
        ev_log_debug("[ev::tools::gltf::GLTFModelManager::prepare_material_descriptor_sets] Adding normal map image binding.");
        texture_layout->add_binding(
            VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            static_cast<uint32_t>(texture_layout->get_bindings().size()),
            1
        );
    }

    CHECK_RESULT(texture_layout->create_layout());
    model->add_descriptor_set_layout(texture_layout);

    for ( auto& material : model->get_materials() ) {
        if ( material->get_base_color_texture() == nullptr ) continue;

        material->set_descriptor_set(
            descriptor_pool->allocate(texture_layout)
        );

        if ( descriptor_binding_flags & DescriptorBindingFlags::ImageBaseColor ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager::prepare_material_descriptor_sets] Writing base color texture to descriptor set.");
            material->get_descriptor_set()->write_texture(
                static_cast<uint32_t>(material->get_descriptor_set()->registry_size()),
                material->get_base_color_texture(),
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            );
        } else if ( material->get_normal_texture() != nullptr && descriptor_binding_flags & DescriptorBindingFlags::ImageNormalMap ) {
            ev_log_debug("[ev::tools::gltf::GLTFModelManager::prepare_material_descriptor_sets] Writing normal map texture to descriptor set.");
            material->get_descriptor_set()->write_texture(
                static_cast<uint32_t>(material->get_descriptor_set()->registry_size()),
                material->get_normal_texture(),
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            );
        }

        material->get_descriptor_set()->update();
    }

    ev_log_info("[ev::tools::gltf::GLTFModelManager::prepare_material_descriptor_sets] Material descriptor sets prepared successfully.");
}

void GLTFModelManager::prepare_node_descriptor_sets(
    std::shared_ptr<ev::tools::gltf::Model> model
) {
    ev_log_info("[ev::tools::gltf::GLTFModelManager::prepare_node_descriptor_sets] Preparing node descriptor sets...");
    std::shared_ptr<ev::DescriptorSetLayout> node_layout
        = std::make_shared<ev::DescriptorSetLayout>(device);

    node_layout->add_binding(
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        0,
        1
    );

    node_layout->create_layout();
    model->add_descriptor_set_layout(node_layout);

    for ( auto node : model->get_nodes() ) {
        prepare_node_descriptor_set(model, node, node_layout);
    }

    ev_log_info("[ev::tools::gltf::GLTFModelManager::prepare_node_descriptor_sets] Node descriptor sets prepared successfully.");
}

void GLTFModelManager::prepare_node_descriptor_set(
    std::shared_ptr<ev::tools::gltf::Model> model,
    std::shared_ptr<ev::tools::gltf::Node> node,
    std::shared_ptr<ev::DescriptorSetLayout> node_layout
) {
    if( node -> get_mesh() ) return;

    node->get_mesh()->set_descriptor_set(
        descriptor_pool->allocate(node_layout)
    );

    std::shared_ptr<ev::DescriptorSet> descriptor_set 
        = node->get_mesh()->get_descriptor_set();

    descriptor_set->write_buffer(
        0,
        node->get_mesh()->get_uniform_buffer(),
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    );
    descriptor_set->update();

    for ( auto& child : node->get_children() ) {
        prepare_node_descriptor_set(model, child, node_layout);
    }
}
