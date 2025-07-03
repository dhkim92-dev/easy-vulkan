#include "ev-sampler.h"

using namespace std;
using namespace ev;

SamplerBuilder& SamplerBuilder::mag_filter(VkFilter mag_filter) {
    m_mag_filter = mag_filter;
    return *this;
}

SamplerBuilder& SamplerBuilder::min_filter(VkFilter min_filter) {
    m_min_filter = min_filter;
    return *this;
}

SamplerBuilder& SamplerBuilder::address_mode_u(VkSamplerAddressMode address_mode) {
    m_address_mode_u = address_mode;
    return *this;
}

SamplerBuilder& SamplerBuilder::address_mode_w(VkSamplerAddressMode address_mode) {
    m_address_mode_w = address_mode;
    return *this;
}

SamplerBuilder& SamplerBuilder::address_mode(VkSamplerAddressMode address_mode) {
    m_address_mode_u = address_mode;
    m_address_mode_v = address_mode; // Assuming you want to set v as well
    m_address_mode_w = address_mode;
    return *this;
}

SamplerBuilder& SamplerBuilder::border_color(VkBorderColor border_color) {
    m_border_color = border_color;
    return *this;
}

SamplerBuilder& SamplerBuilder::mip_lod_bias(float mip_lod_bias) {
    m_mip_lod_bias = mip_lod_bias;
    return *this;
}

SamplerBuilder& SamplerBuilder::max_anisotropy(float max_anisotropy) {
    m_max_anisotropy = max_anisotropy;
    return *this;
}

SamplerBuilder& SamplerBuilder::unnormalized_coordinates(bool unnormalized_coordinates) {
    m_unnormalized_coordinates = unnormalized_coordinates;
    return *this;
}

SamplerBuilder& SamplerBuilder::compare_enable(bool compare_enable) {
    m_compare_enable = compare_enable;
    return *this;
}

SamplerBuilder& SamplerBuilder::compare_op(VkCompareOp compare_op) {
    m_compare_op = compare_op;
    return *this;
}

SamplerBuilder& SamplerBuilder::min_lod(float min_lod) {
    m_min_lod = min_lod;
    return *this;
}

SamplerBuilder& SamplerBuilder::max_lod(float max_lod) {
    m_max_lod = max_lod;
    return *this;
}

SamplerBuilder& SamplerBuilder::flags(VkSamplerCreateFlags flags) {
    m_flags = flags;
    return *this;
}

SamplerBuilder& SamplerBuilder::next(void* next) {
    this->m_next = next;
    return *this;
}

shared_ptr<ev::Sampler> SamplerBuilder::build() {

    return make_shared<ev::Sampler>(
        device,
        m_mag_filter,
        m_min_filter,
        m_address_mode_u,
        m_address_mode_v,
        m_address_mode_w,
        m_border_color,
        m_mip_lod_bias,
        m_max_anisotropy,
        m_unnormalized_coordinates,
        m_compare_enable,
        m_compare_op,
        m_min_lod,
        m_max_lod,
        m_flags,
        m_next
    );
}
