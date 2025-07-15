#include "tools/ev-gltf.h"

using namespace ev::tools::gltf;

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
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] Device is null");
        exit(EXIT_FAILURE);
    }
    if (!this->memory_allocator) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] MemoryAllocator is null");
        exit(EXIT_FAILURE); 
    }
    if (!this->descriptor_pool) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] DescriptorPool is null");
        exit(EXIT_FAILURE);
    }
    if (!this->command_pool) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] CommandPool is null");
        exit(EXIT_FAILURE);
    }
    if (!this->transfer_queue) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] TransferQueue is null");
        exit(EXIT_FAILURE);
    }
}

std::shared_ptr<ev::tools::gltf::Model> GLTFModelManager::load_model(const std::string file_path) {

    tinygltf::Model gltf_model;
    tinygltf::TinyGLTF ctx;

    bool file_loaded = ctx.LoadASCIIFromFile(&gltf_model, nullptr, nullptr, file_path);

    resource_path = std::filesystem::path(file_path).parent_path();
    ev::logger::Logger::getInstance().info("[ev::tools::gltf::GLTFModelManager] Resource path set to: " + resource_path.string());

    if (!file_loaded) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] Failed to load glTF model from file: " + file_path);
        exit(EXIT_FAILURE);
    }
    ev::logger::Logger::getInstance().info("[ev::tools::gltf::GLTFModelManager] Successfully loaded glTF model from file: " + file_path);

    std::shared_ptr<ev::tools::gltf::Model> model = std::make_shared<ev::tools::gltf::Model>(
        device
    );

    // std::vector<uint32_t> h_indices;
    // std::vector<ev::tools::gltf::Vertex> h_vertices;

    load_textures(&gltf_model, model);
    // setup_nodes(&gltf_model, model);
    // setup_skins(&gltf_model, model);
    // setup_materials(&gltf_model, model);
    // setup_animations(&gltf_model, model);
    // setup_textures(&gltf_model, model);

    return model;
}

std::shared_ptr<ev::Texture> GLTFModelManager::load_texture(tinygltf::Image &image, std::string& file_path) {
    ev::logger::Logger::getInstance().info("[ev::tools::gltf::GLTFModelManager::load_texture] Loading image file: " + file_path);
    if (!std::filesystem::exists(file_path)) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager::load_texture] Image file does not exist: " + file_path);
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
        buffer_size = image.image.size();
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
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager::load_texture] Format not supported for blit or sampled image: " + std::to_string(format));
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
    ev::logger::Logger::getInstance().debug("[ev::tools::gltf::GLTFModelManager::load_texture] Texture image created with size: " + std::to_string(width) + "x" + std::to_string(height) + ", mip levels: " + std::to_string(mip_levels));

    std::shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
        device,
        buffer_size,
        ev::buffer_type::STAGING_BUFFER
    );
    ev::logger::Logger::getInstance().debug("[ev::tools::gltf::GLTFModelManager::load_texture] Staging buffer created with size: " + std::to_string(buffer_size));

    if ( memory_allocator->allocate_image(texture_image, ev::memory_type::GPU_ONLY ) != VK_SUCCESS ) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager::load_texture] Failed to allocate image memory for texture.");
        exit(EXIT_FAILURE);
    }
    ev::logger::Logger::getInstance().debug("[ev::tools::gltf::GLTFModelManager::load_texture] Image memory allocated for texture.");

    if ( memory_allocator->allocate_buffer(staging_buffer, ev::memory_type::HOST_READABLE) != VK_SUCCESS ) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager::load_texture] Failed to allocate buffer memory for staging buffer.");
        exit(EXIT_FAILURE);
    }
    ev::logger::Logger::getInstance().debug("[ev::tools::gltf::GLTFModelManager::load_texture] Buffer memory allocated for staging buffer.");

    staging_buffer->map(buffer_size);
    staging_buffer->write(buffer, buffer_size);
    staging_buffer->flush();
    staging_buffer->unmap();
    ev::logger::Logger::getInstance().debug("[ev::tools::gltf::GLTFModelManager::load_texture] Staging buffer created with size: " + std::to_string(buffer_size));

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
    ev::logger::Logger::getInstance().debug("[ev::tools::gltf::GLTFModelManager::load_texture] Image transfer completed.");
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

    ev::logger::Logger::getInstance().debug("[ev::tools::gltf::GLTFModelManager::load_texture] Mipmaps generated for texture image.");

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
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] Failed to create sampler.");
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
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] Failed to create image view.");
        exit(EXIT_FAILURE);
    }

    std::shared_ptr<ev::Texture> texture = std::make_shared<ev::Texture>(
        texture_image,
        image_view,
        sampler
    );

    if (!texture) {
        ev::logger::Logger::getInstance().error("[ev::tools::gltf::GLTFModelManager] Failed to create texture.");
        exit(EXIT_FAILURE);
    }
    return texture;
}

void GLTFModelManager::load_textures(tinygltf::Model* gltf_model, 
    std::shared_ptr<Model> model
) {
    ev::logger::Logger::getInstance().info("[ev::tools::gltf::GLTFModelManager] Loading textures...");  
    for (tinygltf::Image& gltf_image : gltf_model->images) {
        if (gltf_image.uri.empty() && gltf_image.bufferView < 0) {
            ev::logger::Logger::getInstance().warn("[ev::tools::gltf::GLTFModelManager] Image has no URI or buffer view, skipping.");
            continue;
        }

        std::string file_path = resource_path / gltf_image.uri;
        std::shared_ptr<ev::Texture> texture = load_texture(gltf_image, file_path);
        model->get_textures().emplace_back(texture);
    }

    ev::logger::Logger::getInstance().debug("[ev::tools::gltf::GLTFModelManager] Number of textures: " + std::to_string(gltf_model->textures.size()));
}