#include "ev-framebuffer.h" 

using namespace ev;

Framebuffer::Framebuffer(
    std::shared_ptr<Device> _device,
    std::shared_ptr<RenderPass> _render_pass,
    const std::vector<std::shared_ptr<ImageView>>& attachments,
    uint32_t width,
    uint32_t height,
    uint32_t layers
) : device(std::move(_device)), 
    render_pass(std::move(_render_pass)), 
    attachments(attachments), 
    width(width), 
    height(height),
    layers(layers) {
    if (!this->device || !this->render_pass) {
        logger::Logger::getInstance().error("Invalid device or render pass provided for Framebuffer creation.");
        exit(EXIT_FAILURE);
    }

    VkFramebufferCreateInfo framebuffer_ci = {};
    framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.renderPass = *this->render_pass;
    framebuffer_ci.attachmentCount = static_cast<uint32_t>(this->attachments.size());
    std::vector<VkImageView> attachment_views;
    for (const auto& attachment : this->attachments) {
        attachment_views.push_back(*attachment);
    }
    framebuffer_ci.pAttachments = attachment_views.data();
    framebuffer_ci.width = this->width;
    framebuffer_ci.height = this->height;
    framebuffer_ci.layers = this->layers;
    CHECK_RESULT(vkCreateFramebuffer(*this->device, &framebuffer_ci, nullptr, &framebuffer));
}

void Framebuffer::destroy() {
    if (framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(*device, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }
    attachments.clear();
    width = 0;
    height = 0;
}

Framebuffer::~Framebuffer() {
    destroy();
}