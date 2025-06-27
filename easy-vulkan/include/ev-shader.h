#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include "ev-device.h"
#include "ev-logger.h"

namespace ev {

class Shader {

private:

    shared_ptr<Device> device;

    VkShaderModule shader_module = VK_NULL_HANDLE;

    VkShaderStageFlagBits stage;

    std::vector<uint32_t> code;

    std::string entry_point = "main"; // Default entry point name

public:

    explicit Shader(shared_ptr<Device> device, VkShaderStageFlagBits stage, const std::vector<uint32_t>& code);

    Shader(const Shader&) = delete;

    Shader& operator=(const Shader&) = delete;

    void destroy();

    ~Shader();


    operator VkShaderModule() const {
        return shader_module;
    }

    VkPipelineShaderStageCreateInfo get_shader_stage_create_info() const {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = stage,
            .module = shader_module,
            .pName = entry_point.c_str(),
            .pSpecializationInfo = nullptr // Assuming no specialization info for simplicity
        };
    }

    VkShaderStageFlagBits get_stage() const {
        return stage;
    }

    const char* get_entry_point() const {
        return entry_point.c_str();
    }
};

}
