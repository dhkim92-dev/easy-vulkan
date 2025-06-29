#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "ev-shader.h"
#include "ev-device.h"
#include "ev-renderpass.h"
#include "ev-descriptor_set.h"
#include "ev-macro.h"
#include "ev-logger.h"
#include "ev-utility.h"

using namespace std;

namespace ev {

class PipelineCache {

    private:

    shared_ptr<Device> device;  // The Vulkan device associated with this pipeline cache    

    VkPipelineCache cache = VK_NULL_HANDLE;  // The Vulkan pipeline cache handle

    public:

    explicit PipelineCache(shared_ptr<Device> _device, VkPipelineCacheCreateFlags flags = 0, void* next=nullptr);

    ~PipelineCache();

    void destroy();

    operator VkPipelineCache() const {
        return cache;
    }
};

/**
 * @brief Vulkan Pipeline Layout Wrapper
 * VkPipelineLayout의 생성 및 수명주기를 담당합니다.
 * 생성자 호출을 통해 파이프라인을 생성하며, destroy() 메서드를 호출하거나 소멸자를 통해 파이프라인 레이아웃을 소멸합니다.
 */
class PipelineLayout {

private:

    std::shared_ptr<Device> device;  // The Vulkan device associated with this pipeline layout

    VkPipelineLayout layout = VK_NULL_HANDLE;  // The Vulkan pipeline layout handle

    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;  // Descriptor set layouts used in this pipeline layout

    std::vector<VkPushConstantRange> push_constant_ranges;  // Push constant ranges used in this pipeline layout

public:

    /**
     * @brief 생성자
     * @param _device Vulkan Device Wrapper
     * @param descriptor_set_layouts DescriptorSetLayout 객체들의 벡터
     * @param push_constant_ranges Push constant ranges, default is an empty vector
     * 
     */
    explicit PipelineLayout(std::shared_ptr<Device> _device,
        std::vector<std::shared_ptr<DescriptorSetLayout>> descriptor_set_layouts,
        std::vector<VkPushConstantRange> push_constant_ranges = {}
    );

    /**
     * @brief Destroys the pipeline layout and cleans up resources.
     */
    void destroy();

    /**
     * @brief Destructor that cleans up the pipeline layout.
     */
    ~PipelineLayout();

    operator VkPipelineLayout() const {
        return layout;
    }
};

/**
 * @brief Represents a Vulkan graphics pipeline.
 * 오직 파이프라인과 파이프라인 핸들러를 담는 구조체
 * 핸들러의 소유 및 소멸만 담당합니다.
 * 실제 파이프라인 생성 로직은 굉장히 복잡하므로, 별도의 팩토리 클래스를 통해 생성합니다.
 */
class GraphicsPipeline {

private:

    std::shared_ptr<Device> device;

    VkPipeline pipeline = VK_NULL_HANDLE;

    VkPipelineLayout layout = VK_NULL_HANDLE;

public:

    GraphicsPipeline(std::shared_ptr<Device> _device, VkPipeline pipeline, VkPipelineLayout layout);

    ~GraphicsPipeline();

    void destroy();

    operator VkPipeline() const {
        return pipeline;
    }

    operator VkPipelineLayout() const {
        return layout;
    }
};

/**
 * @brief Graphics Pipeline Blueprint
 * 그래픽스 파이프라인을 생성하기 위한 청사진 역할을 합니다ㅓ
 * 이 클래스는 파이프라인의 상태를 정의하고, 실제 파이프라인을 생성하는 팩토리 클래스를 통해 사용됩니다.
 */
struct GraphicsPipelineBluePrint {

public:

    VkPipelineVertexInputStateCreateInfo vertex_input_state_ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

    VkPipelineRasterizationStateCreateInfo rasterization_state_ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO    };

    VkPipelineColorBlendStateCreateInfo color_blend_state_ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};

    VkPipelineViewportStateCreateInfo viewport_state_ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};

    VkPipelineMultisampleStateCreateInfo multisample_state_ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

    VkPipelineDynamicStateCreateInfo dynamic_state_ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO  };

    vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions;

    vector<VkVertexInputBindingDescription> vertex_binding_descriptions;

    vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;

    std::vector<VkDynamicState> dynamic_states;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

    std::shared_ptr<PipelineLayout> pipeline_layout;

    VkPipelineCreateFlags flags = 0;

    vector<float> blend_constants = {0.0f, 0.0f, 0.0f, 0.0f};

    vector<VkViewport> viewports;

    vector<VkRect2D> scissors;

    uint32_t subpass = 0;
};


/**
 * @brief 한 렌더패스에서 공유하는 모든 그래픽스 파이프라인을 생성하기위한 청사진 관리 및 파이프라인 생성 클래스
 * 이 클래스는 여러 그래픽스 파이프라인을 생성하기 위한 청사진을 관리합니다.
 * 각 파이프라인의 상태를 정의한 청사진들의 집합을 이용하여 실제 파이프라인을 관리합니다.
 * 생성된 그래픽스 파이프라인은 각 그래픽스 파이프라인 객체에서 VkPipeline 핸들의 수명을 관리하기 때문에
 * 매니저를 통한 파이프라인 생성 후에는 매니저가 파이프라인 핸들을 소유하지 않습니다.
*/
class GraphicsPipelineBluePrintManager {
    
private:

    std::shared_ptr<Device> device;

    std::shared_ptr<RenderPass> render_pass;

    vector<GraphicsPipelineBluePrint> blue_prints;

    bool on_record = false;

public:

    explicit GraphicsPipelineBluePrintManager(std::shared_ptr<Device> _device, std::shared_ptr<RenderPass> _render_pass);

    GraphicsPipelineBluePrintManager& operator=(const GraphicsPipelineBluePrintManager&) = delete;

    GraphicsPipelineBluePrintManager(const GraphicsPipelineBluePrintManager&) = delete;

    ~GraphicsPipelineBluePrintManager();

    void clear();

    GraphicsPipelineBluePrintManager& begin_blueprint();

    GraphicsPipelineBluePrintManager& add_shader_stage(shared_ptr<Shader> shader);

    GraphicsPipelineBluePrintManager& add_vertex_input_binding_description(
        uint32_t binding,
        uint32_t stride,
        VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX
    );

    GraphicsPipelineBluePrintManager& add_vertex_input_binding_description(VkVertexInputBindingDescription& binding_description);

    GraphicsPipelineBluePrintManager& add_vertex_attribute_description(VkVertexInputAttributeDescription& attribute_description);

    GraphicsPipelineBluePrintManager& add_vertex_attribute_description(
        uint32_t binding,
        uint32_t location,
        VkFormat format,
        uint32_t offset
    );

    // 구조체 내 변수들의 기본값을 지정해서 모두 적기
    GraphicsPipelineBluePrintManager& set_vertex_input_state(VkPipelineVertexInputStateCreateFlags flags = 0, void* next=nullptr);

    GraphicsPipelineBluePrintManager& set_input_assembly_state(
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VkBool32 primitive_restart_enable = VK_FALSE,
        VkPipelineInputAssemblyStateCreateFlags flags = 0,
        void* next = nullptr
    );

    GraphicsPipelineBluePrintManager& set_rasterization_state(
        VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL,
        VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT,
        VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        VkBool32 depth_clamp_enable = VK_FALSE,
        VkBool32 rasterizer_discard_enable = VK_FALSE,
        float line_width = 1.0f,
        VkPipelineRasterizationStateCreateFlags flags = 0,
        void* next = nullptr
    );

    GraphicsPipelineBluePrintManager& set_rasterization_state(
        VkPipelineRasterizationStateCreateInfo& info
    );

    GraphicsPipelineBluePrintManager& set_color_blend_state(
        VkBool32 logic_op_enable = VK_FALSE,
        VkLogicOp logic_op = VK_LOGIC_OP_COPY,
        vector<float> blend_consts = {0.0f, 0.0f, 0.0f, 0.0f},
        VkPipelineColorBlendStateCreateFlags flags = 0,
        void* next = nullptr
    );

    GraphicsPipelineBluePrintManager& add_color_blend_attachment_state(
        VkBool32 blend_enable = VK_FALSE,
        VkBlendFactor src_blend_factor = VK_BLEND_FACTOR_ONE,
        VkBlendFactor dst_blend_factor= VK_BLEND_FACTOR_ZERO,
        VkBlendOp op = VK_BLEND_OP_ADD,
        VkBlendFactor src_alpha_blend_factor = VK_BLEND_FACTOR_ONE,
        VkBlendFactor dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO,
        VkBlendOp alpha_blend_op = VK_BLEND_OP_ADD,
        VkColorComponentFlags color_write_mask = 0xf
    );

    GraphicsPipelineBluePrintManager& add_color_blend_attachment_state(
        const VkPipelineColorBlendAttachmentState& attachmentState
    );



    GraphicsPipelineBluePrintManager& set_viewport_state(
        VkPipelineViewportStateCreateFlags flags = 0,
        void* next = nullptr
    );

    GraphicsPipelineBluePrintManager& add_viewport(uint32_t width, uint32_t height, float minDepth = 0.0f, float maxDepth = 1.0f);

    GraphicsPipelineBluePrintManager& add_scissor(uint32_t width, uint32_t height, int32_t offsetX = 0, int32_t offsetY = 0);

    GraphicsPipelineBluePrintManager& set_multisample_state(
        VkSampleCountFlagBits rasterization_samples = VK_SAMPLE_COUNT_1_BIT,
        VkBool32 sample_shading_enable = VK_FALSE,
        float min_sample_shading = 1.0f,
        const VkSampleMask* sample_mask_optr = nullptr,
        VkBool32 alpha_to_coverage_enable = VK_FALSE,
        VkBool32 alpha_to_one_enable = VK_FALSE,
        VkPipelineMultisampleStateCreateFlags flags = 0,
        void* next = nullptr
    );

    GraphicsPipelineBluePrintManager& set_multisample_state(
        VkPipelineMultisampleStateCreateInfo& info
    );

    GraphicsPipelineBluePrintManager& set_depth_stencil_state(
        VkBool32 depth_test_enable = VK_TRUE,
        VkBool32 depth_write_enable = VK_TRUE,
        VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL,
        VkBool32 depth_bounds_test_enable = VK_FALSE,
        VkBool32 stencil_test_enable = VK_FALSE,
        const VkStencilOpState front = { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
        const VkStencilOpState back = { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
        float min_depth_bounds = 0.0f,
        float max_depth_bounds = 1.0f,
        VkPipelineDepthStencilStateCreateFlags flags = 0,
        void* next = nullptr
    );

    GraphicsPipelineBluePrintManager& set_depth_stencil_state(
        VkPipelineDepthStencilStateCreateInfo& info
    );

    GraphicsPipelineBluePrintManager& set_dynamic_state(
        VkPipelineDynamicStateCreateFlags flags = 0,
        void* next = nullptr
    );

    GraphicsPipelineBluePrintManager& add_dynamic_state(VkDynamicState dynamicState);

    GraphicsPipelineBluePrintManager& set_pipeline_layout(shared_ptr<PipelineLayout> layout);

    GraphicsPipelineBluePrintManager& set_flags(VkPipelineCreateFlags flags);

    GraphicsPipelineBluePrintManager& set_subpass_index(uint32_t subpass);

    GraphicsPipelineBluePrintManager& end_blueprint();

    vector<shared_ptr<GraphicsPipeline>> create_pipelines(shared_ptr<PipelineCache> pipeline_cache = nullptr);
};


} // namespace ev
