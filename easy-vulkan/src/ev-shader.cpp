
#include "ev-shader.h"

using namespace std;
using namespace ev;

Shader::Shader(shared_ptr<Device> _device, VkShaderStageFlagBits stage, const vector<uint32_t>& code)
    : device(std::move(_device)), stage(stage), code(code) {
    ev::logger::Logger::getInstance().info("[ev::Shader] Creating shader module.");
    if (code.empty()) {
        logger::Logger::getInstance().error("Shader code is empty. Cannot create shader module.");
        exit(EXIT_FAILURE);
    }

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size() * sizeof(uint32_t);
    create_info.pCode = code.data();

    if (vkCreateShaderModule(*device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        logger::Logger::getInstance().error("Failed to create shader module.");
        exit(EXIT_FAILURE);
    } 
    logger::Logger::getInstance().info("Shader module created successfully.");
}

void Shader::destroy() {
    ev::logger::Logger::getInstance().info("[ev::Shader::destroy] Destroying shader module.");
    if (shader_module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(*device, shader_module, nullptr);
        shader_module = VK_NULL_HANDLE;
    }
    ev::logger::Logger::getInstance().debug("[ev::Shader::destroy] Shader module destroyed.");
}

Shader::~Shader() {
    destroy();
}

