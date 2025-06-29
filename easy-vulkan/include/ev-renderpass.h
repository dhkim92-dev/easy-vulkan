#pragma once

#include <vector>
#include "ev-device.h"

using namespace std;

namespace ev {


class RenderPass {
private:

    shared_ptr<Device> device;

    VkRenderPass render_pass = VK_NULL_HANDLE;

    const vector<VkAttachmentDescription> attachments;

    const vector<VkSubpassDescription> subpasses;

    const vector<VkSubpassDependency> dependencies;

public:

    explicit RenderPass(
        shared_ptr<Device> device,
        const vector<VkAttachmentDescription> attachments,
        const vector<VkSubpassDescription> subpasses = {},
        const vector<VkSubpassDependency> dependencies = {}
    );

    RenderPass& operator=(const RenderPass&) = delete;

    RenderPass(const RenderPass&) = delete;

    void destroy();

    ~RenderPass();

    operator VkRenderPass() const {
        return render_pass;
    }

};

}