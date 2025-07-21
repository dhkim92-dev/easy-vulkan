#include "ev-pipeline.h"

using namespace std;
using namespace ev;

PipelineCache::PipelineCache(shared_ptr<Device> _device,
    VkPipelineCacheCreateFlags flags,
    void* next ): device(std::move(_device)) {
    ev::logger::Logger::getInstance().info("[ev::PipelineCache] Creating PipelineCache with device: " + std::to_string(reinterpret_cast<uintptr_t>(device.get())));
    if (!device) {
        logger::Logger::getInstance().error("Invalid device provided for PipelineCache creation.");
        exit(EXIT_FAILURE);
    }

    if (cache != VK_NULL_HANDLE) {
        logger::Logger::getInstance().warn("PipelineCache already created, destroying the old cache.");
        return;
    }

    VkPipelineCacheCreateInfo cache_info {};
    cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cache_info.pNext = next;
    cache_info.flags = flags;

    VkResult result = vkCreatePipelineCache(*device, &cache_info, nullptr, &cache);
    if (result != VK_SUCCESS) {
        logger::Logger::getInstance().error("Failed to create pipeline cache: " + std::to_string(result));
        exit(EXIT_FAILURE);
    } else {
        logger::Logger::getInstance().debug("PipelineCache created successfully.");
    }
    ev::logger::Logger::getInstance().info("[ev::PipelineCache] PipelineCache created successfully with handle: " + std::to_string(reinterpret_cast<uintptr_t>(cache)));
}

void PipelineCache::destroy() {
    if (cache != VK_NULL_HANDLE) {
        ev::logger::Logger::getInstance().info("[ev::PipelineCache::destroy] Destroying PipelineCache.");
        vkDestroyPipelineCache(*device, cache, nullptr);
        cache = VK_NULL_HANDLE;
        ev::logger::Logger::getInstance().info("[ev::PipelineCache::destroy] PipelineCache destroyed successfully.");
    }
}

PipelineCache::~PipelineCache() {
    destroy();
}