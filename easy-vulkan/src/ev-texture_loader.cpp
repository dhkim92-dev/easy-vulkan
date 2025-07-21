#include "tools/ev-texture_loader.h"

using namespace ev::tools;

std::vector<uint8_t> TextureLoader::load_data(std::filesystem::path file_path, int& width, int& height, int& channels) {
    ev::logger::Logger::getInstance().info("[TextureLoader::load_data] Loading texture data from file: " + file_path.string());
    
    // Use stb_image to load the image data
    std::vector<uint8_t> data;
    unsigned char* img_data = stbi_load(file_path.string().c_str(), &width, &height, &channels, 0);
    
    if (!img_data) {
        ev::logger::Logger::getInstance().error("[TextureLoader::load_data] Failed to load image: " + file_path.string());
        return data;
    }

    data.assign(img_data, img_data + (width * height * channels));
    stbi_image_free(img_data);
    
    ev::logger::Logger::getInstance().info("[TextureLoader::load_data] Texture data loaded successfully from file: " + file_path.string());
    return data;
}

Texture2DLoader::Texture2DLoader(
    std::shared_ptr<ev::Device> device,
    std::shared_ptr<ev::CommandPool> command_pool,
    std::shared_ptr<ev::Queue> transfer_queue,
    std::shared_ptr<ev::MemoryAllocator> memory_allocator
) : TextureLoader(std::move(device), std::move(command_pool), std::move(transfer_queue), std::move(memory_allocator)) {
    ev::logger::Logger::getInstance().info("[Texture2DLoader::Texture2DLoader] Created Texture2DLoader.");

    if (!m_device || !m_command_pool || !m_transfer_queue || !m_memory_allocator) {
        ev::logger::Logger::getInstance().error("[Texture2DLoader::Texture2DLoader] Invalid parameters provided for Texture2DLoader creation.");
        exit(EXIT_FAILURE);
    }

    ev::logger::Logger::getInstance().info("[Texture2DLoader::Texture2DLoader] Created Texture2DLoader completed");
}

std::shared_ptr<ev::Texture> Texture2DLoader::__load_from_file(
    std::filesystem::path file_path,
    VkFormat format,
    VkImageUsageFlags usage_flags,
    VkImageLayout final_layout,
    uint32_t mip_levels
) {
    ev::logger::Logger::getInstance().info("[Texture2DLoader::load_from_file] Loading texture from file: " + file_path.string());

    int width, height, channels;
    std::vector<uint8_t> data = load_data(file_path, width, height, channels);
    
    if (data.empty()) {
        ev::logger::Logger::getInstance().error("[Texture2DLoader::load_from_file] Failed to load texture data from file: " + file_path.string());
        return nullptr;
    }

    if ( mip_levels == 0 ) {
        mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    }

    std::shared_ptr<ev::Image> image = std::make_shared<ev::Image>(
        m_device,
        VK_IMAGE_TYPE_2D,
        format,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1, // depth
        mip_levels,
        1, // array layers
        usage_flags,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        0, // flags
        VK_SHARING_MODE_EXCLUSIVE,
        0, // queue family count
        nullptr, // queue family indices
        nullptr // pNext
    );

    std::vector<VkBufferImageCopy> copy_regions;
    VkDeviceSize offset = 0;
    for ( uint32_t i = 0 ; i < mip_levels ; ++i ) {
        VkBufferImageCopy region = {};
        region.bufferOffset = offset; // Offset in the buffer
        region.bufferRowLength = 0; // Row length in pixels
        region.bufferImageHeight = 0; // Image height in pixels
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = i;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            static_cast<uint32_t>(width >> i),
            static_cast<uint32_t>(height >> i),
            1 // depth
        };
        copy_regions.push_back(region);
        offset += region.imageExtent.width * region.imageExtent.height * channels;
    }


    VkResult result = m_memory_allocator->allocate_image(image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (result != VK_SUCCESS) {
        ev::logger::Logger::getInstance().error("[Texture2DLoader::load_from_file] Failed to allocate memory for image: " + file_path.string());
        exit(EXIT_FAILURE);
    }

    std::shared_ptr<ev::Buffer> staging_buffer = std::make_shared<ev::Buffer>(
        m_device,
        data.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    );

    result = m_memory_allocator->allocate_buffer(staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (result != VK_SUCCESS) {
        ev::logger::Logger::getInstance().error("[Texture2DLoader::load_from_file] Failed to allocate staging buffer for image: " + file_path.string());
        exit(EXIT_FAILURE);
    }
    staging_buffer->map();
    staging_buffer->write(data.data(), data.size());
    staging_buffer->flush();
    staging_buffer->unmap();
    ev::logger::Logger::getInstance().debug("[Texture2DLoader::load_from_file] Staging buffer created and data written successfully.");

    std::shared_ptr<ev::CommandBuffer> command_buffer = m_command_pool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    if (!command_buffer) {
        ev::logger::Logger::getInstance().error("[Texture2DLoader::load_from_file] Failed to allocate command buffer for image: " + file_path.string());
        exit(EXIT_FAILURE);
    }
    command_buffer->begin();

    ImageMemoryBarrier barrier = ImageMemoryBarrier(
        image,
        0, // src access mask
        VK_ACCESS_TRANSFER_WRITE_BIT, // dst access mask
        VK_IMAGE_LAYOUT_UNDEFINED, // old layout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // new layout
        VK_QUEUE_FAMILY_IGNORED, // src queue family index
        VK_QUEUE_FAMILY_IGNORED, // dst queue family index
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} // subresource range
    );
    command_buffer->pipeline_barrier(
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        {barrier},
        {}, // no buffer barriers
        {} // no memory barriers
    );
    command_buffer->copy_buffer_to_image(
        image,
        staging_buffer,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        copy_regions
    );

    barrier = ImageMemoryBarrier(
        image,
        VK_ACCESS_TRANSFER_WRITE_BIT, // src access mask
        VK_ACCESS_SHADER_READ_BIT, // dst access mask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // old layout
        final_layout, // new layout
        VK_QUEUE_FAMILY_IGNORED, // src queue family index
        VK_QUEUE_FAMILY_IGNORED, // dst queue family index
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, 1} // subresource range
    );
    command_buffer->pipeline_barrier(
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        {barrier},
        {}, // no buffer barriers
        {} // no memory barriers
    );
    command_buffer->end();
    std::shared_ptr<ev::Fence> fence = std::make_shared<ev::Fence>(m_device);
    if (!fence) {
        ev::logger::Logger::getInstance().error("[Texture2DLoader::load_from_file] Failed to create fence for command buffer execution: " + file_path.string());
        exit(EXIT_FAILURE);
    }
    VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
    m_transfer_queue->submit(command_buffer, {}, {}, &wait_stages, fence);
    fence->wait();
    fence->destroy();

    ev::logger::Logger::getInstance().info("[Texture2DLoader::load_from_file] Texture loaded successfully from file: " + file_path.string());

    VkComponentMapping components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, // r
        VK_COMPONENT_SWIZZLE_IDENTITY, // g
        VK_COMPONENT_SWIZZLE_IDENTITY, // b
        VK_COMPONENT_SWIZZLE_IDENTITY  // a
    };

    VkImageSubresourceRange subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
        0, // baseMipLevel
        mip_levels, // levelCount
        0, // baseArrayLayer
        1  // layerCount
    };

    std::shared_ptr<ev::ImageView> image_view = std::make_shared<ev::ImageView>(
        m_device,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        components,
        subresource_range
    );

    std::shared_ptr<ev::Sampler> sampler = 
        std::make_shared<ev::Sampler>(
            m_device,
            VK_FILTER_LINEAR, // magFilter
            VK_FILTER_LINEAR, // minFilter
            VK_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeU
            VK_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeV
            VK_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeW
            VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, // borderColor
            0.0f, // mipLodBias
            1.0f, // maxAnisotropy
            false,
            VK_FALSE, // compareEnable
            VK_COMPARE_OP_NEVER, // compareOp
            0.0f, // minLod
            static_cast<float>(mip_levels), // maxLod
            0,
            nullptr
        );

    return std::make_shared<ev::Texture>(
        image,
        image_view,
        sampler
    );
}

std::shared_ptr<ev::Texture> Texture2DLoader::load_from_file(
    std::filesystem::path file_path,
    VkFormat format,
    VkImageUsageFlags usage_flags,
    VkImageLayout final_layout
) {
    return __load_from_file(file_path, format, usage_flags, final_layout, 0);
}

std::shared_ptr<ev::Texture> Texture2DLoader::load_from_raw_image(
    std::filesystem::path file_path,
    VkFormat format,
    VkImageUsageFlags usage_flags,
    VkImageLayout final_layout
) {
    return __load_from_file(file_path, format, usage_flags, final_layout, 1);
}

std::shared_ptr<ev::Texture> Texture2DLoader::load_from_file_as_uniform(
    std::filesystem::path file_path,
    VkFormat format,
    VkImageUsageFlags usage_flags,
    VkImageLayout final_layout
) {
    ev::logger::Logger::getInstance().info("[Texture2DLoader::load_from_file_as_uniform] Loading texture as uniform from file: " + file_path.string());
ev::logger::Logger::getInstance().info("[Texture2DLoader::load_from_file] Loading texture from file: " + file_path.string());

    int width, height, channels;
    std::vector<uint8_t> data = load_data(file_path, width, height, channels);
    
    if (data.empty()) {
        ev::logger::Logger::getInstance().error("[Texture2DLoader::load_from_file] Failed to load texture data from file: " + file_path.string());
        return nullptr;
    }

    std::shared_ptr<ev::Image> image = std::make_shared<ev::Image>(
        m_device,
        VK_IMAGE_TYPE_2D,
        format,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1, // depth
        0, // mip levels
        1, // array layers
        usage_flags,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        0, // flags
        VK_SHARING_MODE_EXCLUSIVE,
        0, // queue family count
        nullptr, // queue family indices
        nullptr // pNext
    );

    VkResult result = m_memory_allocator->allocate_image(image, ev::memory_type::HOST_READABLE);

    if (result != VK_SUCCESS) {
        ev::logger::Logger::getInstance().error("[Texture2DLoader::load_from_file] Failed to allocate memory for image: " + file_path.string());
        exit(EXIT_FAILURE);
    }

    CHECK_RESULT(image->map(0));
    image->write(data.data(), data.size());
    image->flush();
    image->unmap();


    VkComponentMapping components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, // r
        VK_COMPONENT_SWIZZLE_IDENTITY, // g
        VK_COMPONENT_SWIZZLE_IDENTITY, // b
        VK_COMPONENT_SWIZZLE_IDENTITY  // a
    };

    VkImageSubresourceRange subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
        0, // baseMipLevel
        1, // levelCount
        0, // baseArrayLayer
        1  // layerCount
    };

    std::shared_ptr<ev::ImageView> image_view = std::make_shared<ev::ImageView>(
        m_device,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        components,
        subresource_range
    );

    std::shared_ptr<ev::Sampler> sampler = 
        std::make_shared<ev::Sampler>(
            m_device,
            VK_FILTER_LINEAR, // magFilter
            VK_FILTER_LINEAR, // minFilter
            VK_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeU
            VK_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeV
            VK_SAMPLER_ADDRESS_MODE_REPEAT, // addressModeW
            VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            0.0f,
            1.0f,
            false,
            false,
            VK_COMPARE_OP_NEVER, // compareOp
            0.0f, // minLod
            0.0f, // maxLod
            0,
            nullptr
        );

    return std::make_shared<ev::Texture>(
        image,
        image_view,
        sampler
    );
}
