#include "ev-renderpass.h"

using namespace ev;

RenderPass::RenderPass(
    shared_ptr<Device> _device,
    const vector<VkAttachmentDescription> attachments,
    const vector<VkSubpassDescription> subpasses,
    const vector<VkSubpassDependency> dependencies,
    VkResult *result
) : device(std::move(_device)), attachments(attachments), subpasses(subpasses), dependencies(dependencies) {
    ev_log_info("[ev::RenderPass::RenderPass] Creating RRenderPass with device: %llu", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(device.get())));
    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = static_cast<uint32_t>(this->attachments.size());
    create_info.pAttachments = this->attachments.data();
    create_info.subpassCount = static_cast<uint32_t>(this->subpasses.size());
    create_info.pSubpasses = this->subpasses.data();
    create_info.dependencyCount = static_cast<uint32_t>(this->dependencies.size());
    create_info.pDependencies = this->dependencies.data();
    VkResult res = vkCreateRenderPass(*device, &create_info, nullptr, &render_pass);
    if (result) {
        *result = res;
    } else {
        CHECK_RESULT(res);
    }
    ev_log_info("[ev::RenderPass::RenderPass] RenderPass created successfully. VkRenderPass handle: %llu", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(render_pass)));
}

void RenderPass::destroy() {
    ev_log_info("[ev::RenderPass::destroy] Destroying RenderPass.");
    if (render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(*device, render_pass, nullptr);
        render_pass = VK_NULL_HANDLE;
        ev_log_info("[ev::RenderPass::destroy] RenderPass destroyed successfully.");
    }
    else {
        ev_log_debug("[ev::RenderPass::destroy] RenderPass already destroyed or not initialized.");
    }
}

RenderPass::~RenderPass() {
    destroy();
}