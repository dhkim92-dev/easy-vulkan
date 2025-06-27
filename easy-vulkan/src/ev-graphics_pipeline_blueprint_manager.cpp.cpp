#include "ev-pipeline.h"

using namespace std;
using namespace ev;

GraphicsPipelineBluePrintManager::GraphicsPipelineBluePrintManager(std::shared_ptr<Device> _device,
    std::shared_ptr<RenderPass> _render_pass
)
    : device(std::move(_device)), render_pass(std::move(_render_pass)) {
    if (!device) {
        logger::Logger::getInstance().error("Invalid device provided for GraphicsPipelineBluePrintManager creation.");
        exit(EXIT_FAILURE);
    }
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::begin_blueprint() {
    if (on_record) {
        logger::Logger::getInstance().warn("Already recording a blueprint, finishing the previous one.");
        return *this;
    }
    blue_prints.emplace_back(GraphicsPipelineBluePrint());
    on_record = true;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_shader_stage(shared_ptr<Shader> shader) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    if (!shader) {
        logger::Logger::getInstance().error("Invalid shader provided for shader stage.");
        return *this;
    }
    VkPipelineShaderStageCreateInfo stage_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = shader->get_stage(),
        .module = *shader,
        .pName = shader->get_entry_point(),
        .pSpecializationInfo = nullptr // Assuming no specialization info for simplicity
    };
    blue_prints.back().shader_stages.push_back(shader->get_shader_stage_create_info());
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_vertex_input_state(
    vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
    vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
    VkPipelineVertexInputStateCreateFlags flags
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size()),
        .pVertexBindingDescriptions = vertexBindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
        .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
    };
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_vertex_input_state(
    VkPipelineVertexInputStateCreateInfo& info
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().vertex_input_state = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_input_assembly_state(
    VkPrimitiveTopology topology,
    VkBool32 primitiveRestartEnable,
    VkPipelineInputAssemblyStateCreateFlags flags
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .topology = topology,
        .primitiveRestartEnable = primitiveRestartEnable
    };
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_input_assembly_state(
    VkPipelineInputAssemblyStateCreateInfo& info
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().input_assembly_state = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_rasterization_state(
    VkBool32 depthClampEnable,
    VkBool32 rasterizerDiscardEnable,
    VkPolygonMode polygonMode,
    VkCullModeFlags cullMode,
    VkFrontFace frontFace,
    VkBool32 depthBiasEnable,
    float depthBiasConstantFactor,
    float depthBiasClamp,
    float depthBiasSlopeFactor,
    float lineWidth,
    VkPipelineRasterizationStateCreateFlags flags
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .depthClampEnable = depthClampEnable,
        .rasterizerDiscardEnable = rasterizerDiscardEnable,
        .polygonMode = polygonMode,
        .cullMode = cullMode,
        .frontFace = frontFace,
        .depthBiasEnable = depthBiasEnable,
        .depthBiasConstantFactor = depthBiasConstantFactor,
        .depthBiasClamp = depthBiasClamp,
        .depthBiasSlopeFactor = depthBiasSlopeFactor,
        .lineWidth = lineWidth
    };
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_rasterization_state(
    VkPipelineRasterizationStateCreateInfo& info
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().rasterization_state = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_color_blend_state(
    vector<VkPipelineColorBlendAttachmentState> attachments,
    VkBool32 logicOpEnable,
    VkLogicOp logicOp,
    vector<float> blendConstants,
    VkPipelineColorBlendStateCreateFlags flags
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }

    blue_prints.back().color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .logicOpEnable = logicOpEnable,
        .logicOp = logicOp,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .blendConstants = {blendConstants[0], blendConstants[1], blendConstants[2], blendConstants[3]}
    };
    blue_prints.back().color_blend_attachment_state = attachments.empty() ? VkPipelineColorBlendAttachmentState{} : attachments[0];
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_color_blend_state(
    VkPipelineColorBlendStateCreateInfo& info
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().color_blend_state = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_viewport_state(
    vector<VkViewport> viewports,
    vector<VkRect2D> scissors,
    VkPipelineViewportStateCreateFlags flags
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .viewportCount = static_cast<uint32_t>(viewports.size()),
        .pViewports = viewports.data(),
        .scissorCount = static_cast<uint32_t>(scissors.size()),
        .pScissors = scissors.data()
    };
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_viewport_state(
    VkPipelineViewportStateCreateInfo& info
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().viewport_state = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_multisample_state(
    VkSampleCountFlagBits rasterizationSamples,
    VkBool32 sampleShadingEnable,
    float minSampleShading,
    const VkSampleMask* pSampleMask,
    VkBool32 alphaToCoverageEnable,
    VkBool32 alphaToOneEnable,
    VkPipelineMultisampleStateCreateFlags flags
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .rasterizationSamples = rasterizationSamples,
        .sampleShadingEnable = sampleShadingEnable,
        .minSampleShading = minSampleShading,
        .pSampleMask = pSampleMask,
        .alphaToCoverageEnable = alphaToCoverageEnable,
        .alphaToOneEnable = alphaToOneEnable
    };
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_multisample_state(
    VkPipelineMultisampleStateCreateInfo& info
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().multisample_state = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_depth_stencil_state(
    VkBool32 depthTestEnable,
    VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp,
    VkBool32 depthBoundsTestEnable,
    VkBool32 stencilTestEnable,
    VkStencilOpState front,
    VkStencilOpState back,
    float minDepthBounds,
    float maxDepthBounds,
    VkPipelineDepthStencilStateCreateFlags flags
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .depthTestEnable = depthTestEnable,
        .depthWriteEnable = depthWriteEnable,
        .depthCompareOp = depthCompareOp,
        .depthBoundsTestEnable = depthBoundsTestEnable,
        .stencilTestEnable = stencilTestEnable,
        .front = front,
        .back = back,
        .minDepthBounds = minDepthBounds,
        .maxDepthBounds = maxDepthBounds
    };
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_depth_stencil_state(
    VkPipelineDepthStencilStateCreateInfo& info
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().depth_stencil_state = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_dynamic_state(
    const std::vector<VkDynamicState>& dynamicStates,
    VkPipelineDynamicStateCreateFlags flags
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().dynamic_states = dynamicStates;
    blue_prints.back().dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_dynamic_state(
    VkPipelineDynamicStateCreateInfo& info
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().dynamic_state = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_pipeline_layout(std::shared_ptr<PipelineLayout> layout) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    if (!layout) {
        logger::Logger::getInstance().error("Invalid pipeline layout provided.");
        return *this;
    }
    blue_prints.back().pipeline_layout = layout;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_color_blend_attachment_state(
    VkPipelineColorBlendAttachmentState& attachmentState
) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().color_blend_attachment_state = attachmentState;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_subpass_index(uint32_t subpass) {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().subpass = subpass;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::end_blueprint() {
    if (!on_record) {
        logger::Logger::getInstance().error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    on_record = false;
    return *this;
}

vector<shared_ptr<GraphicsPipeline>> GraphicsPipelineBluePrintManager::create_pipelines(shared_ptr<PipelineCache> pipeline_cache) {
    if (on_record) {
        logger::Logger::getInstance().warn("Blueprint recording is still in progress, finishing it automatically.");
        end_blueprint();
    }


    vector<shared_ptr<GraphicsPipeline>> pipelines;
    vector<VkPipeline> vk_pipelines(this->blue_prints.size());
    vector<VkGraphicsPipelineCreateInfo> pipeline_cis;
    for (const auto& blueprint : blue_prints) {
        if (!blueprint.pipeline_layout) {
            logger::Logger::getInstance().error("Pipeline layout is not set for the blueprint.");
            continue;
        }
        
        VkGraphicsPipelineCreateInfo pipeline_ci = {};
        pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_ci.pNext = nullptr;
        pipeline_ci.flags = blueprint.flags;
        pipeline_ci.stageCount = static_cast<uint32_t>(blueprint.shader_stages.size());
        pipeline_ci.pStages = blueprint.shader_stages.data();
        pipeline_ci.pVertexInputState = &blueprint.vertex_input_state;
        pipeline_ci.pInputAssemblyState = &blueprint.input_assembly_state;
        pipeline_ci.pRasterizationState = &blueprint.rasterization_state;
        pipeline_ci.pColorBlendState = &blueprint.color_blend_state;
        pipeline_ci.pViewportState = &blueprint.viewport_state;
        pipeline_ci.pMultisampleState = &blueprint.multisample_state;
        pipeline_ci.pDepthStencilState = &blueprint.depth_stencil_state;
        pipeline_ci.pDynamicState = &blueprint.dynamic_state;
        pipeline_ci.layout = *blueprint.pipeline_layout;
        pipeline_ci.renderPass = *render_pass; // Render pass should be set later
        pipeline_ci.subpass = blueprint.subpass;
        pipeline_cis.emplace_back(pipeline_ci);
    }

    CHECK_RESULT(vkCreateGraphicsPipelines(*device, *pipeline_cache, static_cast<uint32_t>(pipeline_cis.size()), pipeline_cis.data(), nullptr, vk_pipelines.data()));
    blue_prints.clear();

    for ( size_t i = 0; i < vk_pipelines.size(); ++i ) {
        pipelines.emplace_back(make_shared<GraphicsPipeline>(device, vk_pipelines[i], *blue_prints[i].pipeline_layout));
    }   

    return pipelines;
}

void GraphicsPipelineBluePrintManager::clear() {
    blue_prints.clear();
    on_record = false;
}

GraphicsPipelineBluePrintManager::~GraphicsPipelineBluePrintManager() {
    clear();
}
