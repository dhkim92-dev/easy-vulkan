#pragma once

#include <memory>
#include <vector>
#include "ev-device.h"
#include "ev-image.h"
#include "ev-logger.h"


using namespace std;

namespace ev {

class Sampler {
private:
    shared_ptr<ev::Device> device = nullptr;
    VkSampler sampler = VK_NULL_HANDLE;

public:
    explicit Sampler(
        shared_ptr<ev::Device> device,
        VkFilter mag_filter = VK_FILTER_LINEAR,
        VkFilter min_filter = VK_FILTER_LINEAR,
        VkSamplerAddressMode address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VkSamplerAddressMode address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VkSamplerAddressMode address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VkBorderColor border_color = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        float mip_lod_bias = 0.0f,
        float max_anisotropy = 1.0f,
        bool unnormalized_coordinates = false,
        bool compare_enable = false,
        VkCompareOp compare_op = VK_COMPARE_OP_ALWAYS,
        float min_lod = 0.0f,
        float max_lod = VK_LOD_CLAMP_NONE,
        VkSamplerCreateFlags flags = 0, 
        void* next = nullptr
    );

    Sampler(const Sampler&) = delete;

    Sampler& operator=(const Sampler&) = delete;

    ~Sampler();

    void destroy();

    operator VkSampler() const {
        return sampler;
    }
};


class SamplerBuilder {

private:

    shared_ptr<ev::Device> device = nullptr;

    VkFilter m_mag_filter = VK_FILTER_LINEAR;

    VkFilter m_min_filter = VK_FILTER_LINEAR;

    VkSamplerAddressMode m_address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkSamplerAddressMode m_address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkSamplerAddressMode m_address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkBorderColor m_border_color = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    float m_mip_lod_bias = 0.0f;

    float m_max_anisotropy = 1.0f;

    bool m_unnormalized_coordinates = false;

    bool m_compare_enable = false;

    VkCompareOp m_compare_op = VK_COMPARE_OP_ALWAYS;

    float m_min_lod = 0.0f;

    float m_max_lod = VK_LOD_CLAMP_NONE;

    VkSamplerCreateFlags m_flags = 0;

    void* m_next = nullptr;

public:
    explicit SamplerBuilder(shared_ptr<ev::Device> device) : device(device) {}

    SamplerBuilder& mag_filter(VkFilter mag_filter);

    SamplerBuilder& min_filter(VkFilter min_filter);

    SamplerBuilder& address_mode_u(VkSamplerAddressMode address_mode);

    SamplerBuilder& address_mode_w(VkSamplerAddressMode address_mode);

    SamplerBuilder& address_mode(VkSamplerAddressMode address_mode);

    SamplerBuilder& border_color(VkBorderColor border_color);

    SamplerBuilder& mip_lod_bias(float mip_lod_bias);

    SamplerBuilder& max_anisotropy(float max_anisotropy);

    SamplerBuilder& unnormalized_coordinates(bool unnormalized_coordinates);

    SamplerBuilder& compare_enable(bool compare_enable);

    SamplerBuilder& compare_op(VkCompareOp compare_op);

    SamplerBuilder& min_lod(float min_lod);

    SamplerBuilder& max_lod(float max_lod);

    SamplerBuilder& next(void* next);

    SamplerBuilder& flags(VkSamplerCreateFlags flags);

    shared_ptr<ev::Sampler> build();
};
}