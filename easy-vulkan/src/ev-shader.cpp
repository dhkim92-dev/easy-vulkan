
#include "ev-shader.h"

using namespace std;
using namespace ev;

Shader::Shader(shared_ptr<Device> _device, VkShaderStageFlagBits stage, const vector<uint32_t>& code)
    : device(std::move(_device)), stage(stage), code(code) {
    if (code.empty()) {
        throw runtime_error("Shader code cannot be empty.");
    }

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size() * sizeof(uint32_t);
    create_info.pCode = code.data();

    if (vkCreateShaderModule(*device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw runtime_error("Failed to create shader module.");
    }
}

void Shader::destroy() {
    if (shader_module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(*device, shader_module, nullptr);
        shader_module = VK_NULL_HANDLE;
    }
}

Shader::~Shader() {
    destroy();
}

