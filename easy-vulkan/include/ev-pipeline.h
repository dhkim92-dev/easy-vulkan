#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "ev-device.h"
#include "ev-descriptor_set.h"
#include "ev-logger.h"
#include "ev-utility.h"

using namespace std;

namespace ev {

/**
 * @brief Represents a Vulkan pipeline layout.
 * This class encapsulates the creation and management of a Vulkan pipeline layout,
 * which includes descriptor sets and push constants.
 */
class PipelineLayout {

private:

    std::shared_ptr<Device> device;  // The Vulkan device associated with this pipeline layout

    VkPipelineLayout layout = VK_NULL_HANDLE;  // The Vulkan pipeline layout handle

    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;  // Descriptor set layouts used in this pipeline layout

    std::vector<VkPushConstantRange> push_constant_ranges;  // Push constant ranges used in this pipeline layout

public:

    /**
     * @brief Constructs a PipelineLayout with the given device.
     * @param _device The Vulkan device to use for this pipeline layout.
     */
    explicit PipelineLayout(std::shared_ptr<Device> _device,
        std::vector<std::shared_ptr<DescriptorSetLayout> > descriptor_set_layouts,
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

} // namespace ev