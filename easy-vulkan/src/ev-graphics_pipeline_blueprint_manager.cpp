#include "ev-pipeline.h"
#include <algorithm>

using namespace std;
using namespace ev;

GraphicsPipelineBluePrintManager::GraphicsPipelineBluePrintManager(std::shared_ptr<Device> _device,
    std::shared_ptr<RenderPass> _render_pass
)
    : device(std::move(_device)), render_pass(std::move(_render_pass)) {
    if (!device) {
        ev_log_error("Invalid device provided for GraphicsPipelineBluePrintManager creation.");
        exit(EXIT_FAILURE);
    }
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::begin_blueprint() {
    if (on_record) {
        ev_log_warn("Already recording a blueprint, finishing the previous one.");
        return *this;
    }
    blue_prints.emplace_back(GraphicsPipelineBluePrint());
    on_record = true;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_shader_stage(shared_ptr<Shader> shader) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    if (!shader) {
        ev_log_error("Invalid shader provided for shader stage.");
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

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_vertex_input_binding_description(
    uint32_t binding,
    uint32_t stride,
    VkVertexInputRate input_rate) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }

    VkVertexInputBindingDescription description = {};
    description.binding = binding; // Default binding index
    description.stride = stride; // Default stride, should be set later
    description.inputRate = input_rate; // Default input rate
    this->blue_prints.back().vertex_binding_descriptions.push_back(description);
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_vertex_input_binding_description(
    VkVertexInputBindingDescription& binding_description
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().vertex_binding_descriptions.push_back(binding_description);
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_vertex_attribute_description(
    uint32_t binding,
    uint32_t location,
    VkFormat format,
    uint32_t offset
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    VkVertexInputAttributeDescription attribute_description = {};
    attribute_description.location = location;
    attribute_description.binding = binding;
    attribute_description.format = format;
    attribute_description.offset = offset;
    blue_prints.back().vertex_attribute_descriptions.push_back(attribute_description);
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_vertex_attribute_description(
    VkVertexInputAttributeDescription& attribute_description
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().vertex_attribute_descriptions.push_back(attribute_description);
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_vertex_input_state(
    VkPipelineVertexInputStateCreateFlags flags,
    void* next
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }

    blue_prints.back().vertex_input_state_ci.flags = flags;
    blue_prints.back().vertex_input_state_ci.pNext = next;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_vertex_input_state(
    VkPipelineVertexInputStateCreateInfo* info
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().vertex_input_state_ci = *info;
    blue_prints.back().skip_vertex_input_state_ci = true;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_input_assembly_state(
    VkPrimitiveTopology topology,
    VkBool32 primitiveRestartEnable,
    VkPipelineInputAssemblyStateCreateFlags flags,
    void* next
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    auto& info = blue_prints.back().input_assembly_state_ci;
    info.topology = topology;
    info.primitiveRestartEnable = primitiveRestartEnable;
    info.flags = flags;
    info.pNext = next;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_rasterization_state(
    VkPipelineRasterizationStateCreateInfo& info
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().rasterization_state_ci = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_rasterization_state(
    VkPolygonMode polygon_mode,
    VkCullModeFlags cull_mode,
    VkFrontFace front_face,
    VkBool32 depth_clamp_enable,
    VkBool32 rasterizer_discard_enable,
    float line_width,
    VkPipelineRasterizationStateCreateFlags flags,
    void* next
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    auto& ci = blue_prints.back().rasterization_state_ci;
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    ci.pNext = next;
    ci.flags = flags;
    ci.polygonMode = polygon_mode;
    ci.cullMode = cull_mode;
    ci.frontFace = front_face;
    ci.depthClampEnable = depth_clamp_enable;
    ci.rasterizerDiscardEnable = rasterizer_discard_enable;
    ci.lineWidth = line_width;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_color_blend_state(
    VkBool32 logic_op_enable,
    VkLogicOp logic_op,
    vector<float> blend_constants,
    VkPipelineColorBlendStateCreateFlags flags,
    void* next
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    auto& ci = blue_prints.back().color_blend_state_ci;
    ci.logicOpEnable = logic_op_enable;
    ci.logicOp = logic_op;
    ci.flags = flags;
    ci.pNext = next;
    std::copy(blue_prints.back().blend_constants.begin(), blue_prints.back().blend_constants.end(), ci.blendConstants);
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_color_blend_attachment_state(
    VkBool32 blend_enable,
    VkBlendFactor src_blend_factor,
    VkBlendFactor dst_blend_factor,
    VkBlendOp op,
    VkBlendFactor src_alpha_blend_factor,
    VkBlendFactor dst_alpha_blend_factor,
    VkBlendOp alpha_blend_op,
    VkColorComponentFlags color_write_mask
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().color_blend_attachments.push_back({
        .blendEnable = blend_enable,
        .srcColorBlendFactor = src_blend_factor,
        .dstColorBlendFactor = dst_blend_factor,
        .colorBlendOp = op,
        .srcAlphaBlendFactor = src_alpha_blend_factor,
        .dstAlphaBlendFactor = dst_alpha_blend_factor,
        .alphaBlendOp = alpha_blend_op,
        .colorWriteMask = color_write_mask
    });
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_color_blend_attachment_state(
    const VkPipelineColorBlendAttachmentState& attachment_state
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().color_blend_attachments.push_back(attachment_state);

    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_viewport_state(
    VkPipelineViewportStateCreateFlags flags,
    void * next
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    auto& ci = blue_prints.back().viewport_state_ci;
    ci.pNext = next;
    ci.flags = flags;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_viewport(
    uint32_t width, uint32_t height, float min_depth, float max_depth
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().viewports.push_back({
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .minDepth = min_depth,
        .maxDepth = max_depth
    });
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_scissor(
    uint32_t width, uint32_t height, int32_t offsetX, int32_t offsetY
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().scissors.push_back({
        .offset = {offsetX, offsetY},
        .extent = {width, height}
    });
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_multisample_state(
    VkSampleCountFlagBits rasterization_samples,
    VkBool32 sample_shading_enable,
    float min_sample_shading,
    const VkSampleMask* p_sample_mask,
    VkBool32 alpha_to_coverage_enable,
    VkBool32 alpha_to_one_enable,
    VkPipelineMultisampleStateCreateFlags flags,
    void* next
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    auto& ci = blue_prints.back().multisample_state_ci;
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ci.pNext = next;
    ci.flags = flags;
    ci.rasterizationSamples = rasterization_samples;
    ci.sampleShadingEnable = sample_shading_enable;
    ci.minSampleShading = min_sample_shading;
    ci.pSampleMask = p_sample_mask;
    ci.alphaToCoverageEnable = alpha_to_coverage_enable;
    ci.alphaToOneEnable = alpha_to_one_enable;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_multisample_state(
    VkPipelineMultisampleStateCreateInfo& info
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    auto& ci = blue_prints.back().multisample_state_ci;
    ci = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_depth_stencil_state(
    VkBool32 depth_test_enable,
    VkBool32 depth_write_enable,
    VkCompareOp depth_compare_op,
    VkBool32 depth_bounds_test_enable,
    VkBool32 stencil_test_enable,
    const VkStencilOpState front,
    const VkStencilOpState back,
    float min_depth_bounds,
    float max_depth_bounds,
    VkPipelineDepthStencilStateCreateFlags flags,
    void* next
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    auto& info = blue_prints.back().depth_stencil_state_ci;
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.pNext = next;
    info.flags = flags;
    info.depthTestEnable = depth_test_enable;
    info.depthWriteEnable = depth_write_enable;
    info.depthCompareOp = depth_compare_op;
    info.depthBoundsTestEnable = depth_bounds_test_enable;
    info.stencilTestEnable = stencil_test_enable;
    info.front = front;
    info.back = back;
    info.minDepthBounds = min_depth_bounds;
    info.maxDepthBounds = max_depth_bounds;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_depth_stencil_state(
    VkPipelineDepthStencilStateCreateInfo& info
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().depth_stencil_state_ci = info;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_dynamic_state(
    VkPipelineDynamicStateCreateFlags flags,
    void* next
) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    auto& ci = blue_prints.back().dynamic_state_ci;
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    ci.pNext = nullptr;
    ci.flags = flags;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::add_dynamic_state(VkDynamicState state) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().dynamic_states.push_back(state);
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_pipeline_layout(std::shared_ptr<PipelineLayout> layout) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    if (!layout) {
        ev_log_error("Invalid pipeline layout provided.");
        return *this;
    }
    blue_prints.back().pipeline_layout = layout;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::set_subpass_index(uint32_t subpass) {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    blue_prints.back().subpass = subpass;
    return *this;
}

GraphicsPipelineBluePrintManager& GraphicsPipelineBluePrintManager::end_blueprint() {
    if (!on_record) {
        ev_log_error("No blueprint is being recorded, call begin_blueprint() first.");
        return *this;
    }
    on_record = false;
    return *this;
}

vector<shared_ptr<GraphicsPipeline>> GraphicsPipelineBluePrintManager::create_pipelines(shared_ptr<PipelineCache> pipeline_cache) {
    if (on_record) {
        ev_log_warn("Blueprint recording is still in progress, finishing it automatically.");
        end_blueprint();
    }


    vector<shared_ptr<GraphicsPipeline>> pipelines;
    vector<VkPipeline> vk_pipelines(this->blue_prints.size());
    vector<VkGraphicsPipelineCreateInfo> pipeline_cis;
    for (auto& blueprint : blue_prints) {
        if (!blueprint.pipeline_layout) {
            ev_log_error("Pipeline layout is not set for the blueprint.");
            continue;
        }
        
        VkGraphicsPipelineCreateInfo pipeline_ci = {};
        pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_ci.pNext = nullptr;
        pipeline_ci.flags = blueprint.flags;
        pipeline_ci.stageCount = static_cast<uint32_t>(blueprint.shader_stages.size());
        pipeline_ci.pStages = blueprint.shader_stages.data();

        /* --- Vertex Input State Create Info Setting --- */
        if (!blueprint.skip_vertex_input_state_ci) {
            blueprint.vertex_input_state_ci.pVertexAttributeDescriptions = blueprint.vertex_attribute_descriptions.data();
            blueprint.vertex_input_state_ci.vertexAttributeDescriptionCount = static_cast<uint32_t>(blueprint.vertex_attribute_descriptions.size());
            blueprint.vertex_input_state_ci.pVertexBindingDescriptions = blueprint.vertex_binding_descriptions.data();
            blueprint.vertex_input_state_ci.vertexBindingDescriptionCount = static_cast<uint32_t>(blueprint.vertex_binding_descriptions.size());
        } 
        pipeline_ci.pVertexInputState = &blueprint.vertex_input_state_ci;

        /* --- Input Assembly State Create Info Setting --- */
        pipeline_ci.pInputAssemblyState = &blueprint.input_assembly_state_ci;

        /* --- Rasterization State Create Info Setting --- */
        pipeline_ci.pRasterizationState = &blueprint.rasterization_state_ci;

        /* --- Color Blend State Create Info Setting --- */
        blueprint.color_blend_state_ci.attachmentCount = static_cast<uint32_t>(blueprint.color_blend_attachments.size());
        blueprint.color_blend_state_ci.pAttachments = blueprint.color_blend_attachments.data();
        blueprint.color_blend_state_ci.blendConstants[0] = blueprint.blend_constants[0];
        blueprint.color_blend_state_ci.blendConstants[1] = blueprint.blend_constants[1];
        blueprint.color_blend_state_ci.blendConstants[2] = blueprint.blend_constants[2];
        blueprint.color_blend_state_ci.blendConstants[3] = blueprint.blend_constants[3];
        pipeline_ci.pColorBlendState = &blueprint.color_blend_state_ci;

        /* --- Viewport State Create Info Setting --- */
        if (blueprint.rasterization_state_ci.rasterizerDiscardEnable == VK_TRUE ) {
            pipeline_ci.pViewportState = nullptr;
        } else {
            blueprint.viewport_state_ci.viewportCount = static_cast<uint32_t>(blueprint.viewports.size());
            blueprint.viewport_state_ci.pViewports = blueprint.viewports.data();
            blueprint.viewport_state_ci.scissorCount = static_cast<uint32_t>(blueprint.scissors.size());
            blueprint.viewport_state_ci.pScissors = blueprint.scissors.data();
            // ev_log_debug("Viewport count : " + std::to_string(blueprint.viewport_state_ci.viewportCount));
            // ev_log_debug("Scissor count : " + std::to_string(blueprint.viewport_state_ci.scissorCount));
            pipeline_ci.pViewportState = &blueprint.viewport_state_ci;
        }
        pipeline_ci.pMultisampleState = &blueprint.multisample_state_ci;
        pipeline_ci.pDepthStencilState = &blueprint.depth_stencil_state_ci;

        if ( blueprint.dynamic_states.empty() ) {
            pipeline_ci.pDynamicState = nullptr; // No dynamic states
        } else {
            blueprint.dynamic_state_ci.dynamicStateCount = static_cast<uint32_t>(blueprint.dynamic_states.size());
            blueprint.dynamic_state_ci.pDynamicStates = blueprint.dynamic_states.data();
            pipeline_ci.pDynamicState = &blueprint.dynamic_state_ci;
        }
        pipeline_ci.layout = *blueprint.pipeline_layout;
        pipeline_ci.renderPass = *render_pass;  
        pipeline_ci.subpass = blueprint.subpass;
        pipeline_cis.emplace_back(pipeline_ci);
    }

    CHECK_RESULT(vkCreateGraphicsPipelines(*device, *pipeline_cache, static_cast<uint32_t>(pipeline_cis.size()), pipeline_cis.data(), nullptr, vk_pipelines.data()));

    for ( size_t i = 0; i < vk_pipelines.size(); ++i ) {
        pipelines.emplace_back(make_shared<GraphicsPipeline>(device, vk_pipelines[i], *blue_prints[i].pipeline_layout));
    }   

    blue_prints.clear();
    return pipelines;
}

void GraphicsPipelineBluePrintManager::clear() {
    blue_prints.clear();
    on_record = false;
}

GraphicsPipelineBluePrintManager::~GraphicsPipelineBluePrintManager() {
    clear();
}
