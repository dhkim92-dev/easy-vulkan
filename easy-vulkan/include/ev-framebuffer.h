#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "ev-device.h"
#include "ev-image.h"
#include "ev-image_view.h"
#include "ev-renderpass.h"
#include "ev-logger.h"

using namespace std;

namespace ev {

/** 
 * @brief Vulkan Framebuffer Wrapper
 * @note 이 클래스는 Vulkan Framebuffer를 래핑합니다.
 *       Framebuffer는 렌더 패스와 연결된 이미지 뷰들의 집합으로, 
 *       렌더링 결과를 저장하는 역할을 합니다.
*/
class Framebuffer {

private:

    std::shared_ptr<Device> device;

    std::shared_ptr<RenderPass> render_pass;

    // std::vector<std::shared_ptr<ImageView>> attachments;
    std::vector<VkImageView> attachments;

    std::vector<std::shared_ptr<ev::ImageView>> image_views;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;

    uint32_t width;

    uint32_t height;

    uint32_t layers = 1; // Default to 1 layer for 2D images

public:

    // explicit Framebuffer(
    //     std::shared_ptr<Device> device,
    //     std::shared_ptr<RenderPass> render_pass,
    //     const std::vector<shared_ptr<ImageView>> attachments,
    //     uint32_t width,
    //     uint32_t height,
    //     uint32_t layers = 1
    // );

    /**
     * @brief 프레임버퍼 생성자
     * @param device Vulkan Device Wrapper
     * @param render_pass Vulkan RenderPass Wrapper
     * @param attachments 이미지 뷰들의 벡터
     * @param width 프레임버퍼의 너비
     * @param height 프레임버퍼의 높이
     * @param layers 프레임버퍼의 레이어 수 (기본값: 1)
     */
    explicit Framebuffer(
        std::shared_ptr<Device> device,
        std::shared_ptr<RenderPass> render_pass,
        const std::vector<VkImageView> attachments,
        uint32_t width,
        uint32_t height,
        uint32_t layers = 1
    );

    explicit Framebuffer(
        std::shared_ptr<Device> device,
        std::shared_ptr<RenderPass> render_pass,
        const std::vector<std::shared_ptr<ev::ImageView>> attachments,
        uint32_t width,
        uint32_t height,
        uint32_t layers = 1
    );

    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(const Framebuffer&) = delete;

    void destroy();

    ~Framebuffer();

    uint32_t get_width() const {
        return width;
    }

    uint32_t get_height() const {
        return height;
    }

    uint32_t get_layers() const {
        return layers;
    }

    operator VkFramebuffer() const {
        return framebuffer;
    }

};

}