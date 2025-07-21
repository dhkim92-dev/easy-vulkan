#include "ev-pipeline.h"

namespace ev {

ComputePipeline::ComputePipeline(std::shared_ptr<ev::Device> _device)
    : device(std::move(_device)) {
    ev::logger::Logger::getInstance().info("[ComputePipeline] Creating ComputePipeline with default constructor.");
    if (!device) {
        ev::logger::Logger::getInstance().error("[ComputePipeline::ComputePipeline] : Invalid device provided for ComputePipeline creation.");
        exit(EXIT_FAILURE);
    }
}

ComputePipeline::ComputePipeline(std::shared_ptr<ev::Device> _device, 
    std::shared_ptr<ev::PipelineLayout> layout, 
    const std::shared_ptr<ev::Shader> shader
)
    : device(std::move(_device)), layout(std::move(layout)), shader(std::move(shader)) {
    if (!device) {
        ev::logger::Logger::getInstance().error("[ComputePipeline::ComputePipeline] : Invalid device provided for ComputePipeline creation.");
        exit(EXIT_FAILURE);
    }
    if (!layout) {
        ev::logger::Logger::getInstance().error("[ComputePipeline::ComputePipeline] : Invalid pipeline layout provided for ComputePipeline creation.");
        exit(EXIT_FAILURE);
    }

    if ( !shader ) {
        ev::logger::Logger::getInstance().error("[ComputePipeline::ComputePipeline] : Invalid shader provided for ComputePipeline creation.");
        exit(EXIT_FAILURE);
    }

    if ( shader->get_stage() != VK_SHADER_STAGE_COMPUTE_BIT ) {
        ev::logger::Logger::getInstance().error("[ComputePipeline::ComputePipeline] : Shader stage must be VK_SHADER_STAGE_COMPUTE_BIT for ComputePipeline.");
        exit(EXIT_FAILURE);
    }
}

VkResult ComputePipeline::create_pipeline(
    std::shared_ptr<ev::PipelineCache> pipeline_cache,
    VkPipelineCreateFlags flags,
    void* next
) {
    if ( !device || !layout || !shader ) {
        ev::logger::Logger::getInstance().error("[ComputePipeline::create_pipeline] : Device, layout or shader is not set.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkComputePipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.stage = shader->get_shader_stage_create_info();
    info.layout = *layout;
    info.flags = flags; // No special flags
    info.basePipelineHandle = VK_NULL_HANDLE; // No base pipeline
    info.basePipelineIndex = -1; // No base pipeline index
    info.pNext = next;

    return vkCreateComputePipelines(
        *device, 
        pipeline_cache ? VkPipelineCache(*pipeline_cache) : VK_NULL_HANDLE,
        1, 
        &info, 
        nullptr, 
        &pipeline
    );
}

void ComputePipeline::destroy() {
    ev::logger::Logger::getInstance().info("[ComputePipeline::destroy] : Destroying compute pipeline: " + std::to_string(reinterpret_cast<uintptr_t>(pipeline)));
    if (pipeline != VK_NULL_HANDLE) {
        ev::logger::Logger::getInstance().info("[ComputePipeline::destroy] : Destroying compute pipeline : " + std::to_string(reinterpret_cast<uintptr_t>(pipeline)));
        vkDestroyPipeline(*device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    ev::logger::Logger::getInstance().info("[ComputePipeline::destroy] : Compute pipeline destroyed successfully.");
}

ComputePipeline::~ComputePipeline() {
    destroy();
}

}