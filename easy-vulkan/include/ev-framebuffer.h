#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "ev-device.h"
#include "ev-image.h"
#include "ev-image-view.h"
#include "ev-renderpass.h"
#include "ev-logger.h"

using namespace std;

namespace ev {

class Framebuffer {

private:

    std::shared_ptr<Device> device;

    std::shared_ptr<RenderPass> render_pass;

    std::vector<std::shared_ptr<ImageView>> attachments;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;

    uint32_t width;

    uint32_t height;

    uint32_t layers = 1; // Default to 1 layer for 2D images

public:

    explicit Framebuffer(
        std::shared_ptr<Device> device,
        std::shared_ptr<RenderPass> render_pass,
        const std::vector<std::shared_ptr<ImageView>>& attachments,
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