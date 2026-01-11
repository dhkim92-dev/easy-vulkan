#include <gtest/gtest.h>
#include <easy-vulkan.h>
#include <memory>
#include <filesystem>
#include "test_common.h"

using namespace std;
using namespace ev;

class ShaderTest : public ::testing::Test {
protected:
    shared_ptr<Instance> instance;
    shared_ptr<PhysicalDevice> physical_device;
    shared_ptr<Device> device;

    void SetUp() override {
        instance = std::make_shared<ev::Instance>(std::vector<const char*>(), std::vector<const char*>(), false);
        physical_device = std::make_shared<ev::PhysicalDevice>(instance, ev::utility::list_physical_devices(*instance)[0]);
        device = std::make_shared<ev::Device>(instance, physical_device, std::vector<const char*>());
    }
};

TEST_F(ShaderTest, CreateShaderModule) {
    //vector<uint32_t> shader_code = {0x07230203, 0x00010000, 0x0008000a, 0x00000000}; // Minimal SPIR-V code
    vector<uint32_t> shader_code;
    std::filesystem::path base_path = get_executable_dir();
    auto vertex_shader_path = base_path / "shaders" / "vert.spv";
    ev::utility::read_spirv_shader_file(vertex_shader_path.c_str(), shader_code);
    Shader shader(device, VK_SHADER_STAGE_VERTEX_BIT, shader_code);
    EXPECT_NE(VkShaderModule(shader), VK_NULL_HANDLE);
    EXPECT_EQ(shader.get_stage(), VK_SHADER_STAGE_VERTEX_BIT);
}
