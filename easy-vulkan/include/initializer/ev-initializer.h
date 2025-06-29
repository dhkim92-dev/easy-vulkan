#pragma once

#include <vulkan/vulkan.h>
#include <vector>
using namespace std;

namespace ev {
namespace initializer {

    inline VkApplicationInfo create_application_info(const char* applicationName, 
        uint32_t applicationVersion,
        const char* engineName, 
        uint32_t engineVersion, 
        uint32_t apiVersion) {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = applicationName;
        appInfo.applicationVersion = applicationVersion;
        appInfo.pEngineName = engineName;
        appInfo.engineVersion = engineVersion;
        appInfo.apiVersion = apiVersion;
        return appInfo;
    }

    inline VkInstanceCreateInfo instance_create_info(
        const VkApplicationInfo* pApplicationInfo = nullptr, 
        uint32_t enabledLayerCount = 0, 
        const char* const* ppEnabledLayerNames = nullptr, 
        uint32_t enabledExtensionCount = 0, 
        const char* const* ppEnabledExtensionNames = nullptr
    ) {
        VkInstanceCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pNext = nullptr;
        ci.flags = 0;
        ci.pApplicationInfo = pApplicationInfo;
        ci.enabledLayerCount = enabledLayerCount;
        ci.ppEnabledLayerNames = ppEnabledLayerNames;
        ci.enabledExtensionCount = enabledExtensionCount;
        ci.ppEnabledExtensionNames = ppEnabledExtensionNames;
        return ci;
    }

    inline VkDeviceCreateInfo device_create_info() {
        VkDeviceCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        return ci;
    }

    inline VkVertexInputAttributeDescription vertex_input_attribute_description(
        uint32_t location = 0,
        uint32_t binding = 0,
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
        uint32_t offset = 0
    ) {
        VkVertexInputAttributeDescription desc = {};
        desc.location = location;
        desc.binding = binding;
        desc.format = format;
        desc.offset = offset;
        return desc;
    }

    inline VkVertexInputBindingDescription vertex_input_binding_description(
        uint32_t binding,
        uint32_t stride,
        VkVertexInputRate rate
    ) {
        VkVertexInputBindingDescription desc = {};
        desc.binding = binding;
        desc.stride = stride;
        desc.inputRate = rate;
        return desc;
    }

    inline VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info(
        vector<VkVertexInputBindingDescription> const& pVertexBindingDescriptions,
        vector<VkVertexInputAttributeDescription> const& pVertexAttributeDescriptions,
        VkPipelineVertexInputStateCreateFlags flags = 0,
        void* pNext = nullptr
    ) {
        VkPipelineVertexInputStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info.pNext = pNext;
        info.flags = flags;
        info.vertexBindingDescriptionCount = static_cast<uint32_t>(pVertexBindingDescriptions.size());
        info.pVertexBindingDescriptions = pVertexBindingDescriptions.data();
        info.vertexAttributeDescriptionCount = static_cast<uint32_t>(pVertexAttributeDescriptions.size());
        info.pVertexAttributeDescriptions = pVertexAttributeDescriptions.data();
        return info;
    }

    inline VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VkBool32 primitive_restart = VK_FALSE,
        VkPipelineInputAssemblyStateCreateFlags flags = 0,
        void* next = nullptr
    ) {
        VkPipelineInputAssemblyStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        info.pNext = next;
        info.flags = flags;
        info.topology = topology;
        info.primitiveRestartEnable = primitive_restart;
        return info;
    }

    inline VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info(
        vector<VkViewport> const& viewports,
        vector<VkRect2D> const& scissors,
        VkPipelineViewportStateCreateFlags flags = 0,
        void* pNext = nullptr
    ) {
        VkPipelineViewportStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        info.pNext = pNext;
        info.flags = flags;
        info.viewportCount = static_cast<uint32_t>(viewports.size());
        info.scissorCount = static_cast<uint32_t>(scissors.size());
        info.pViewports = viewports.data();
        info.pScissors = scissors.data();
        return info;
    }

    inline VkViewport viewport(
        float x = 0.0f, 
        float y = 0.0f, 
        float width = 1.0f, 
        float height = 1.0f, 
        float min_depth = 0.0f, 
        float max_depth = 1.0f
    ) {
        VkViewport vp = {};
        vp.x = x;
        vp.y = y;
        vp.width = width;
        vp.height = height;
        vp.minDepth = min_depth;
        vp.maxDepth = max_depth;
        return vp;
    }

    inline VkRect2D scissor(
        int32_t offsetX = 0, 
        int32_t offsetY = 0, 
        uint32_t width = 1, 
        uint32_t height = 1
    ) {
        VkRect2D rect = {};
        rect.offset.x = offsetX;
        rect.offset.y = offsetY;
        rect.extent.width = width;
        rect.extent.height = height;
        return rect;
    }

    inline VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL,
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT,
        VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE,
        VkBool32 depthClampEnable = VK_FALSE,
        VkBool32 rasterizerDiscardEnable = VK_FALSE,
        float lineWidth = 1.0f,
        VkPipelineRasterizationStateCreateFlags flags = 0,
        void* pNext = nullptr
    ) {
        VkPipelineRasterizationStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        info.pNext = pNext;
        info.flags = flags;
        info.depthClampEnable = depthClampEnable;
        info.rasterizerDiscardEnable = rasterizerDiscardEnable;
        info.polygonMode = polygonMode;
        info.cullMode = cullMode;
        info.frontFace = frontFace;
        info.lineWidth = lineWidth;
        return info;
    }

    inline VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info(
        VkSampleCountFlagBits rasterization_samples = VK_SAMPLE_COUNT_1_BIT,
        VkBool32 sample_shading_enable = VK_FALSE,
        float min_sample_shading = 1.0f,
        const VkSampleMask* sample_mask_ptr = nullptr,
        VkBool32 alpha_to_coverage_enable = VK_FALSE,
        VkBool32 alpha_to_one_enable = VK_FALSE,
        VkPipelineMultisampleStateCreateFlags flags = 0,
        void* pNext = nullptr
    ) {
        VkPipelineMultisampleStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        info.pNext = pNext;
        info.flags = flags;
        info.rasterizationSamples = rasterization_samples;
        info.sampleShadingEnable = sample_shading_enable;
        info.minSampleShading = min_sample_shading;
        info.pSampleMask = sample_mask_ptr;
        info.alphaToCoverageEnable = alpha_to_coverage_enable;
        info.alphaToOneEnable = alpha_to_one_enable;
        return info;
    }

    inline VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(
        VkBool32 blendEnable = VK_FALSE,
        VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        VkBlendOp colorBlendOp = VK_BLEND_OP_ADD,
        VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD,
        VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                               VK_COLOR_COMPONENT_G_BIT | 
                                               VK_COLOR_COMPONENT_B_BIT | 
                                               VK_COLOR_COMPONENT_A_BIT
    ) {
        VkPipelineColorBlendAttachmentState attachment = {};
        attachment.blendEnable = blendEnable;
        attachment.srcColorBlendFactor = srcColorBlendFactor;
        attachment.dstColorBlendFactor = dstColorBlendFactor;
        attachment.colorBlendOp = colorBlendOp;
        attachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
        attachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
        attachment.alphaBlendOp = alphaBlendOp;
        attachment.colorWriteMask = colorWriteMask;
        return attachment;
    }

    inline VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
        vector<VkPipelineColorBlendAttachmentState> const& attachments,
        VkPipelineColorBlendStateCreateFlags flags = 0,
        void* next = nullptr
    ) {
        VkPipelineColorBlendStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        info.pNext = next;
        info.flags = flags;
        info.logicOpEnable = VK_FALSE;
        info.attachmentCount = static_cast<uint32_t>(attachments.size());
        info.pAttachments = attachments.data();
        return info;
    }

    inline VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info(
        VkBool32 depthTestEnable = VK_TRUE,
        VkBool32 depthWriteEnable = VK_TRUE,
        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS,
        VkBool32 depthBoundsTestEnable = VK_FALSE,
        VkBool32 stencilTestEnable = VK_FALSE,
        VkStencilOpState front = {},
        VkStencilOpState back = {},
        float minDepthBounds = 0.0f,
        float maxDepthBounds = 1.0f,
        VkPipelineDepthStencilStateCreateFlags flags = 0,
        void* pNext = nullptr
    ) {
        VkPipelineDepthStencilStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        info.pNext = pNext;
        info.flags = flags;
        info.depthTestEnable = depthTestEnable;
        info.depthWriteEnable = depthWriteEnable;
        info.depthCompareOp = depthCompareOp;
        info.depthBoundsTestEnable = depthBoundsTestEnable;
        info.stencilTestEnable = stencilTestEnable;
        info.front = front;
        info.back = back;
        info.minDepthBounds = minDepthBounds;
        info.maxDepthBounds = maxDepthBounds;
        return info;
    }

    // 앞으로 제공하는 코드는 모두 snake case로 작성합니다.
    inline VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(
        vector<VkDynamicState> const& dynamic_states,
        VkPipelineDynamicStateCreateFlags flags = 0,
        void* next = nullptr) {
        VkPipelineDynamicStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        info.pNext = next;
        info.flags = flags;
        info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        info.pDynamicStates = dynamic_states.data();
        return info;
    }

    inline VkStencilOpState stencil_op_state(
        VkStencilOp failOp = VK_STENCIL_OP_KEEP,
        VkStencilOp passOp = VK_STENCIL_OP_KEEP,
        VkStencilOp depthFailOp = VK_STENCIL_OP_KEEP,
        VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS,
        uint32_t compareMask = 0,
        uint32_t writeMask = 0,
        uint32_t reference = 0
    ) {
        VkStencilOpState state = {};
        state.failOp = failOp;
        state.passOp = passOp;
        state.depthFailOp = depthFailOp;
        state.compareOp = compareOp;
        state.compareMask = compareMask;
        state.writeMask = writeMask;
        state.reference = reference;
        return state;
    }
}
}