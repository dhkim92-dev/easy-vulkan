#include "ev-pipeline.h"

using namespace std;
using namespace ev;

GraphicsPipeline::GraphicsPipeline(shared_ptr<Device> _device, 
    VkPipeline _pipeline, 
    VkPipelineLayout _layout
) : device(std::move(_device)), pipeline(_pipeline), layout(_layout) {
    logger::Logger::getInstance().debug("Pipeline created successfully.");
}

void GraphicsPipeline::destroy() {
    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(*device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
}

GraphicsPipeline::~GraphicsPipeline() {
    destroy();
}