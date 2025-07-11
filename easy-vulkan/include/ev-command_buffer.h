#pragma once

#include <vector>
#include <memory>
#include "ev-device.h"
#include "ev-utility.h"
#include "ev-logger.h"
#include "ev-renderpass.h"
#include "ev-framebuffer.h"
#include "ev-descriptor_set.h"
#include "ev-pipeline.h"
#include "ev-sync.h"

using namespace std;

namespace ev {

class CommandBuffer {

private:

    shared_ptr<Device> device ;

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;

    VkCommandPool command_pool = VK_NULL_HANDLE;

    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

public:

    explicit CommandBuffer(shared_ptr<Device> _device, VkCommandPool command_pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    explicit CommandBuffer(shared_ptr<Device> _device, VkCommandPool command_pool, VkCommandBuffer command_buffer); 

    CommandBuffer& operator=(const CommandBuffer&) = delete;

    CommandBuffer(const CommandBuffer&) = delete;

    ~CommandBuffer();

    void destroy();

    void begin_render_pass(shared_ptr<RenderPass> render_pass, 
        shared_ptr<Framebuffer> framebuffer, 
        vector<VkClearValue> clear_values = {},
        VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

    void end_render_pass();
         

    void bind_descriptor_sets(VkPipelineBindPoint pipeline_bind_point, 
        shared_ptr<PipelineLayout> layout, 
        vector<shared_ptr<DescriptorSet>> descriptor_sets,
        uint32_t first_set = 0,
        const vector<uint32_t> dynamic_offsets = {}
    );

    void bind_vertex_buffers(uint32_t first_binding, 
        const vector<shared_ptr<Buffer>> buffers, 
        const vector<VkDeviceSize> offsets = {});

    void bind_index_buffers(vector<shared_ptr<Buffer>> buffers, VkDeviceSize offset = 0, VkIndexType index_type = VK_INDEX_TYPE_UINT32);

    void bind_graphics_pipeline(shared_ptr<GraphicsPipeline> pipeline);

    // VkResult bind_compute_pipeline(shared_ptr<ComputePipeline> &pipeline); TODO: ComputePipeline 구현 후 구현

    // VkResult bind_ray_tracing_pipeline(shared_ptr<RayTracingPipeline> &pipeline); TODO: RayTracingPipeline 구현 후 구현

    void bind_push_constants(shared_ptr<PipelineLayout> layout, VkShaderStageFlags stage_flags, uint32_t offset, const void* data, size_t size);

    void copy_buffer(shared_ptr<Buffer> dst_buffer, shared_ptr<Buffer> src_buffer, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize dst_offset = 0, VkDeviceSize src_offset = 0);
    
    void copy_buffer_to_image(shared_ptr<Image> dst_image, 
        shared_ptr<Buffer> src_buffer, 
        VkImageLayout image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        const vector<VkBufferImageCopy> regions = {}
    );

    void copy_image(shared_ptr<Image> dst_image, 
        shared_ptr<Image> src_image, 
        VkImageLayout dst_image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VkImageLayout src_image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        uint32_t region_count = 0,
        const vector<VkImageCopy> regions = { }
    );

    void copy_image_to_buffer(shared_ptr<Image> src_image, 
        shared_ptr<Buffer> dst_buffer, 
        VkImageLayout src_image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        const vector<VkBufferImageCopy>& regions = {}
    );

    void draw(uint32_t vertex_count, 
        uint32_t instance_count = 1, 
        uint32_t first_vertex = 0, 
        uint32_t first_instance = 0) ;

    void draw_indexed(uint32_t index_count, 
        uint32_t instance_count = 1, 
        uint32_t first_index = 0, 
        int32_t vertex_offset = 0, 
        uint32_t first_instance = 0);

    void set_viewport(
        float offset_x,
        float offset_y,
        float width,
        float height,
        float min_depth = 0.0f,
        float max_depth = 1.0f
    );

    void set_viewports(
        const vector<VkViewport> viewports
    );

    void set_scissor(
        uint32_t offset_x,
        uint32_t offset_y,
        uint32_t width,
        uint32_t height
    );

    void set_scissors(
        const vector<VkRect2D> scissors
    );

    void dispatch(uint32_t group_count_x, uint32_t group_count_y = 1, uint32_t group_count_z = 1);

    // VkResult wait_event() // TODO: Event 구현 후 구현
    // VkResult reset_event(shared_ptr<Event> &event) // TODO: Event 구현 후 구현
    // VkResult signal_event(shared_ptr<Event> &event) // TODO: Event 구현 후 구현

    // VkResult barrier() {
        // vkCmdPipelineBarrier()
    // }

    VkResult pipeline_barrier(
        std::shared_ptr<ev::Image> image,
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask,
        const vector<ImageMemoryBarrier> image_memory_barriers = {},
        const vector<BufferMemoryBarrier> buffer_memory_barriers = {},
        const vector<MemoryBarrier> memory_barriers = {}
    ) {
        vector<VkImageMemoryBarrier> vk_image_memory_barriers;
        vk_image_memory_barriers.reserve(image_memory_barriers.size());
        for (const auto& imb : image_memory_barriers) {
            vk_image_memory_barriers.push_back(static_cast<VkImageMemoryBarrier>(imb));
        }
        vector<VkBufferMemoryBarrier> vk_buffer_memory_barriers;
        vk_buffer_memory_barriers.reserve(buffer_memory_barriers.size());
        for (const auto& bmb : buffer_memory_barriers) {
            vk_buffer_memory_barriers.push_back(static_cast<VkBufferMemoryBarrier>(bmb));
        }
        vector<VkMemoryBarrier> vk_memory_barriers;
        vk_memory_barriers.reserve(memory_barriers.size());
        for (const auto& mb : memory_barriers) {
            vk_memory_barriers.push_back(static_cast<VkMemoryBarrier>(mb));
        }

        vkCmdPipelineBarrier(
            command_buffer,
            src_stage_mask,
            dst_stage_mask,
            0,
            static_cast<uint32_t>(vk_memory_barriers.size()),
            vk_memory_barriers.data(),
            static_cast<uint32_t>(vk_buffer_memory_barriers.size()),
            vk_buffer_memory_barriers.data(),
            static_cast<uint32_t>(vk_image_memory_barriers.size()),
            vk_image_memory_barriers.data()
        );

        return VK_SUCCESS;
    }

    VkResult begin(VkCommandBufferUsageFlags flags = 0);

    VkResult end();

    VkResult reset(VkCommandBufferResetFlags flags = 0);

#if defined(VK_VERSION_1_4) 
#endif

    operator VkCommandBuffer() const {
        return command_buffer;
    }
};

}