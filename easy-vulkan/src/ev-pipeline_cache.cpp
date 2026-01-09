#include "ev-pipeline.h"

using namespace std;
using namespace ev;

PipelineCache::PipelineCache(shared_ptr<Device> _device,
    VkPipelineCacheCreateFlags flags,
    void* next ): device(std::move(_device)) {
    ev_log_info("[ev::PipelineCache] Creating PipelineCache with device: %llu", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(device.get())));
    if (!device) {
        ev_log_error("Invalid device provided for PipelineCache creation.");
        exit(EXIT_FAILURE);
    }

    if (cache != VK_NULL_HANDLE) {
        ev_log_warn("PipelineCache already created, destroying the old cache.");
        return;
    }

    VkPipelineCacheCreateInfo cache_info {};
    cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cache_info.pNext = next;
    cache_info.flags = flags;

    VkResult result = vkCreatePipelineCache(*device, &cache_info, nullptr, &cache);
    if (result != VK_SUCCESS) {
        ev_log_error("Failed to create pipeline cache: %d", result);
        exit(EXIT_FAILURE);
    } else {
        ev_log_debug("PipelineCache created successfully.");
    }
    ev_log_info("[ev::PipelineCache] PipelineCache created successfully with handle: %llu", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(cache)));
}

void PipelineCache::destroy() {
    if (cache != VK_NULL_HANDLE) {
        ev_log_info("[ev::PipelineCache::destroy] Destroying PipelineCache.");
        vkDestroyPipelineCache(*device, cache, nullptr);
        cache = VK_NULL_HANDLE;
        ev_log_info("[ev::PipelineCache::destroy] PipelineCache destroyed successfully.");
    }
}

PipelineCache::~PipelineCache() {
    destroy();
}