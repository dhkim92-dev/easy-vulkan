#pragma once

#include <memory>
#include <filesystem>
#include "stb_image.h"
#include "ev-device.h"
#include "ev-image.h"
#include "ev-image_view.h"
#include "ev-sampler.h"
#include "ev-texture.h"
#include "ev-command_pool.h"
#include "ev-command_buffer.h"
#include "ev-queue.h"
#include "ev-memory_allocator.h"

#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
#endif

namespace ev::tools {

class TextureLoader {

    protected:

    std::shared_ptr<ev::Device> m_device;

    std::shared_ptr<ev::CommandPool> m_command_pool;

    std::shared_ptr<ev::Queue> m_transfer_queue;

    std::shared_ptr<ev::MemoryAllocator> m_memory_allocator;

    TextureLoader(std::shared_ptr<ev::Device> device, 
                    std::shared_ptr<ev::CommandPool> command_pool,
                    std::shared_ptr<ev::Queue> transfer_queue,
                    std::shared_ptr<ev::MemoryAllocator> memory_allocator)
        : m_device(std::move(device)), 
          m_command_pool(std::move(command_pool)), 
          m_transfer_queue(std::move(transfer_queue)), 
          m_memory_allocator(std::move(memory_allocator)) {}

    std::vector<uint8_t> load_data(std::filesystem::path file_path, int& width, int& height, int& channels);

    public :
    
    virtual std::shared_ptr<ev::Texture> load_from_file(
        std::filesystem::path file_path,
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
        VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) = 0;

    virtual ~TextureLoader() = default; // 가상 소멸자(권장)
};

class Texture2DLoader : public TextureLoader {

    private :

    std::shared_ptr<ev::Texture> __load_from_file(
        std::filesystem::path file_path,
        VkFormat format,
        VkImageUsageFlags usage_flags,
        VkImageLayout final_layout,
        uint32_t mip_levels = 0
    );

    public: 

    explicit Texture2DLoader(std::shared_ptr<ev::Device> device,
                            std::shared_ptr<ev::CommandPool> command_pool,
                            std::shared_ptr<ev::Queue> transfer_queue,
                            std::shared_ptr<ev::MemoryAllocator> memory_allocator);

    Texture2DLoader(const Texture2DLoader&) = delete;

    ~Texture2DLoader() = default;

    /**
     * @brief 텍스처 파일을 로드합니다. 텍스처 전용 이미지로 제작된 경우 사용합니다. 
     * @param file_path 파일 경로
     * @param format 이미지 포맷 (기본값: VK_FORMAT_R8G8B8A8_UNORM)
     * @param usage_flags 이미지 사용 플래그 (기본값: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
     * @param final_layout 최종 이미지 레이아웃 (기본값: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
     * @return std::shared_ptr<ev::Texture> 로드된 텍스처 객체
     * @details mip map은 자동으로 계산되며, Device Local 메모리에 할당됩니다.  
     */
    std::shared_ptr<ev::Texture> load_from_file(
        std::filesystem::path file_path,
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
        VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) override;

    /**
     * @brief 텍스처 전용 이미지가 아닌 경우 사용합니다. 
     * @param data 이미지 데이터
     * @param format 이미지 포맷 (기본값: VK_FORMAT_R8G8B8A8_UNORM)
     * @param usage_flags 이미지 사용 플래그 (기본값: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
     * @param final_layout 최종 이미지 레이아웃 (기본값: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
     * @return std::shared_ptr<ev::Texture> 로드된 텍스처 객체
     * @details Device Local 메모리에 할당됩니다.
     */
    std::shared_ptr<ev::Texture> load_from_raw_image(
        std::filesystem::path file_path,
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
        VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    /**
     * @brief 유니폼 텍스처로 사용하기 위한 이미지 로더입니다.
     * @param format 이미지 포맷 (기본값: VK_FORMAT_R8G8B8A8_UNORM)
     * @param usage_flags 이미지 사용 플래그 (기본값: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
     * @param final_layout 최종 이미지 레이아웃 (기본값: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
     * @return std::shared_ptr<ev::Texture> 로드된 텍스처 객체
     */
    std::shared_ptr<ev::Texture> load_from_file_as_uniform(
        const std::filesystem::path file_path,
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
        VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VkImageLayout final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
};

}

