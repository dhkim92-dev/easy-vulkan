#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "ev-device.h"

namespace ev {

class Shader {

private:

    shared_ptr<Device> device;

    VkShaderModule shader_module = VK_NULL_HANDLE;

    VkShaderStageFlagBits stage;

    std::vector<uint32_t> code;

public:

    explicit Shader(shared_ptr<Device> device, VkShaderStageFlagBits stage, const std::vector<uint32_t>& code);

    Shader(const Shader&) = delete;

    Shader& operator=(const Shader&) = delete;

    void destroy();

    ~Shader();

    VkShaderModule get_shader_module() const {
        return shader_module;
    }

    VkShaderStageFlagBits get_stage() const {
        return stage;
    }

};

}
