#include "ev-pipeline.h"

namespace ev {

PipelineLayout::PipelineLayout(std::shared_ptr<Device> _device,
    std::vector<std::shared_ptr<DescriptorSetLayout>> descriptor_set_layouts,
    std::vector<VkPushConstantRange> push_constant_ranges
): device(std::move(_device)) {
    logger::Logger::getInstance().info("[ev::PipelineLayout::PipelineLayout] Creating PipelineLayout with device: " + std::to_string(reinterpret_cast<uintptr_t>(device.get())));
    // Create the pipeline layout here
    if (!device) {
        logger::Logger::getInstance().error("Invalid device provided for PipelineLayout creation.");
        exit(EXIT_FAILURE);
    }

    if ( layout != VK_NULL_HANDLE ) {
        logger::Logger::getInstance().warn("PipelineLayout already created, destroying the old layout.");
        return;
    }

    VkPipelineLayoutCreateInfo layout_info = {};
    vector<VkDescriptorSetLayout> vk_descriptor_set_layouts;
    for (const auto& descriptor_set_layout : descriptor_set_layouts) {
        vk_descriptor_set_layouts.emplace_back(*descriptor_set_layout);
    }
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = static_cast<uint32_t>(vk_descriptor_set_layouts.size());
    layout_info.pSetLayouts = vk_descriptor_set_layouts.data();
    layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
    layout_info.pPushConstantRanges = push_constant_ranges.data();
    layout_info.flags = 0; // No special flags for now
    CHECK_RESULT(vkCreatePipelineLayout(*device, &layout_info, nullptr, &layout));
    ev::logger::Logger::getInstance().info("[ev::PipelineLayout::PipelineLayout] Created PipelineLayout successfully.");
}

void PipelineLayout::destroy() {
    if (layout != VK_NULL_HANDLE) {
        ev::logger::Logger::getInstance().info("[ev::PipelineLayout::destroy] Destroying PipelineLayout.");
        vkDestroyPipelineLayout(*device, layout, nullptr);
        layout = VK_NULL_HANDLE;
        ev::logger::Logger::getInstance().info("[ev::PipelineLayout::destroy] PipelineLayout destroyed successfully.");
    }
}

PipelineLayout::~PipelineLayout() {
    destroy();
}

} // namespace ev