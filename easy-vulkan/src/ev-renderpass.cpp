#include "ev-renderpass.h"

using namespace ev;

RenderPass::RenderPass(
    shared_ptr<Device> _device,
    const vector<VkAttachmentDescription>& attachments,
    const vector<VkSubpassDescription>& subpasses,
    const vector<VkSubpassDependency>& dependencies
) : device(std::move(_device)), attachments(attachments), subpasses(subpasses), dependencies(dependencies) {
    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = static_cast<uint32_t>(this->attachments.size());
    create_info.pAttachments = this->attachments.data();
    create_info.subpassCount = static_cast<uint32_t>(this->subpasses.size());
    create_info.pSubpasses = this->subpasses.data();
    create_info.dependencyCount = static_cast<uint32_t>(this->dependencies.size());
    create_info.pDependencies = this->dependencies.data();
    CHECK_RESULT(vkCreateRenderPass(*device, &create_info, nullptr, &render_pass));
}

void RenderPass::destroy() {
    if (render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(*device, render_pass, nullptr);
        render_pass = VK_NULL_HANDLE;
    }
}

RenderPass::~RenderPass() {
    destroy();
}