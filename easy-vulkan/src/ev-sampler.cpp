#include "ev-sampler.h"

using namespace std;
using namespace ev;

Sampler::Sampler(
    shared_ptr<ev::Device> device,
    VkFilter mag_filter,
    VkFilter min_filter,
    VkSamplerAddressMode address_mode_u,
    VkSamplerAddressMode address_mode_v,
    VkSamplerAddressMode address_mode_w,
    VkBorderColor border_color,
    float mip_lod_bias,
    float max_anisotropy,
    bool unnormalized_coordinates,
    bool compare_enable,
    VkCompareOp compare_op,
    float min_lod,
    float max_lod,
    VkSamplerCreateFlags flags,
    void* next
) : device(device) {
    if (!device) {
        logger::Logger::getInstance().error("[ev::Sampler] Invalid device provided for Sampler creation.");
        exit(EXIT_FAILURE);
    }

    logger::Logger::getInstance().info("[ev::Sampler] Creating sampler with device: " + to_string(reinterpret_cast<uintptr_t>(device.get())));
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = mag_filter;
    sampler_info.minFilter = min_filter;
    sampler_info.addressModeU = address_mode_u;
    sampler_info.addressModeV = address_mode_v;
    sampler_info.addressModeW = address_mode_w;
    sampler_info.borderColor = border_color;
    sampler_info.unnormalizedCoordinates = unnormalized_coordinates ? VK_TRUE : VK_FALSE;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = mip_lod_bias;
    sampler_info.maxAnisotropy = max_anisotropy;
    sampler_info.anisotropyEnable = max_anisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    sampler_info.compareEnable = compare_enable ? VK_TRUE : VK_FALSE;
    sampler_info.compareOp = compare_op;
    sampler_info.minLod = min_lod;
    sampler_info.maxLod = max_lod;
    sampler_info.flags = flags;
    sampler_info.pNext = next;
    CHECK_RESULT(vkCreateSampler(*device, &sampler_info, nullptr, &sampler));
    logger::Logger::getInstance().info("[ev::Sampler] Sampler created successfully with handle: " + to_string(reinterpret_cast<uintptr_t>(sampler)));
}

Sampler::~Sampler() {
    destroy();
}

void Sampler::destroy() {
    ev::logger::Logger::getInstance().info("[ev::Sampler::destroy] Destroying sampler.");
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(*device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    ev::logger::Logger::getInstance().info("[ev::Sampler::destroy] Sampler destroyed successfully.");
}