#include "ev-command_buffer.h"

using namespace std;
using namespace ev;

/**
 * @brief CommandBuffer 생성자
 * @param _device : Device 객체의 shared_ptr
 * @param command_pool : CommandPool 객체의 VkCommandPool 핸들
 * @param level : CommandBuffer의 레벨 (기본값: VK_COMMAND_BUFFER_LEVEL_PRIMARY)
 * 이 생성자는 CommandPool을 이용하여 하나의 커맨드 버퍼를 할당하는 경우 사용합니다.
 */
CommandBuffer::CommandBuffer(shared_ptr<Device> _device, VkCommandPool command_pool, VkCommandBufferLevel level)
    : device(std::move(_device)), command_pool(command_pool), level(level) {
    ev_log_info("[CommandBuffer::CommandBuffer] : Creating CommandBuffer with device: %p, command pool: %p", device.get(), reinterpret_cast<void*>(command_pool));
    if (!device) {
        ev_log_error("[CommandBuffer::CommandBuffer] : Invalid device provided for CommandBuffer creation.");
        exit(EXIT_FAILURE);
    }

    
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = level;
    alloc_info.commandBufferCount = 1;

    CHECK_RESULT(vkAllocateCommandBuffers(*device, &alloc_info, &command_buffer));
    ev_log_info("[CommandBuffer::CommandBuffer] : CommandBuffer created successfully.");
}

/**
 * @brief CommandBuffer 생성자
 * @param _device : Device 객체의 shared_ptr
 * @param command_pool : CommandPool 객체의 VkCommandPool 핸들
 * @param command_buffer : 이미 할당된 VkCommandBuffer 핸들
 * 이 생성자는 한번에 대량의 CommandPool을 이용하여 VkCommandBuffer를 할당하는 경우 사용합니다.
 */
CommandBuffer::CommandBuffer(shared_ptr<Device> _device, VkCommandPool command_pool, VkCommandBuffer command_buffer)
: device(std::move(_device)), command_pool(command_pool), command_buffer(command_buffer) {
    if (!device) {
        ev_log_error("[CommandBuffer::CommandBuffer] : Invalid device provided for CommandBuffer creation.");
        exit(EXIT_FAILURE);
    }
    
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::CommandBuffer] : Invalid command buffer handle provided.");
        exit(EXIT_FAILURE);
    }
}

void CommandBuffer::begin_render_pass(shared_ptr<RenderPass> render_pass, 
    shared_ptr<Framebuffer> framebuffer,
    vector<VkClearValue> clear_values,
    VkSubpassContents contents
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::begin_render_pass] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    VkRenderPassBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = *render_pass;
    begin_info.framebuffer = *framebuffer;
    begin_info.renderArea.offset = {0, 0};
    begin_info.renderArea.extent.width = framebuffer->get_width();
    begin_info.renderArea.extent.height = framebuffer->get_height();
    begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    begin_info.pClearValues = clear_values.data();
    vkCmdBeginRenderPass(command_buffer, &begin_info, contents);
}

void CommandBuffer::blit_image(
    shared_ptr<Image> src_image, 
    VkImageLayout src_image_layout, 
    shared_ptr<Image> dst_image, 
    VkImageLayout dst_image_layout, 
    const vector<VkImageBlit> regions,
    VkFilter filter
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::blit_image] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    
    vkCmdBlitImage(command_buffer, 
        *src_image, src_image_layout, 
        *dst_image, dst_image_layout, 
        static_cast<uint32_t>(regions.size()), regions.data(), 
        filter);
}

void CommandBuffer::end_render_pass() {
    vkCmdEndRenderPass(command_buffer);
}

void CommandBuffer::bind_descriptor_sets(
    VkPipelineBindPoint pipeline_bind_point,
    shared_ptr<PipelineLayout> layout,
    vector<shared_ptr<DescriptorSet>> descriptor_sets,
    uint32_t first_set,
    const vector<uint32_t> dynamic_offsets
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::bind_descriptor_sets] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    
    vector<VkDescriptorSet> sets;
    for (const auto& ds : descriptor_sets) {
        sets.push_back(*ds);
    }

    vkCmdBindDescriptorSets(command_buffer, 
        pipeline_bind_point, 
        *layout, 
        first_set, 
        static_cast<uint32_t>(sets.size()), sets.data(), 
        static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());
}

void CommandBuffer::bind_vertex_buffers(
    uint32_t first_binding,
    const vector<shared_ptr<Buffer>> buffers,
    const vector<VkDeviceSize> offsets
) {
    ev_log_debug("[CommandBuffer::bind_vertex_buffer] : Binding vertex buffers to command buffer with size : %s", std::to_string(buffers.size()).c_str());
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::bind_vertex_buffer] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }

    vector<VkBuffer> vk_buffers;
    for (const auto& buffer : buffers) {
        VkBuffer vb = *buffer;
        vk_buffers.push_back(vb);
    }
    ev_log_debug("[CommandBuffer::bind_vertex_buffer] : Binding vertex buffers to command buffer with vk size : %d", static_cast<int>(vk_buffers.size()));

    vkCmdBindVertexBuffers(command_buffer, first_binding, static_cast<uint32_t>(vk_buffers.size()), vk_buffers.data(), offsets.data());
}

void CommandBuffer::bind_index_buffers(
    vector<shared_ptr<Buffer>> buffers, 
    VkDeviceSize offset, 
    VkIndexType index_type
) {
    ev_log_debug("[CommandBuffer::bind_index_buffers] : Binding index buffers to command buffer with size : %d", static_cast<int>(buffers.size()));
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::bind_index_buffers] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }

    if (buffers.empty()) {
        ev_log_error("[CommandBuffer::bind_index_buffers] : No buffers provided for binding index buffers.");
        return;
    }

    vkCmdBindIndexBuffer(command_buffer, *buffers[0], offset, index_type);
}

void CommandBuffer::bind_graphics_pipeline(shared_ptr<GraphicsPipeline> pipeline) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::bind_graphics_pipeline] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
}

void CommandBuffer::bind_push_constants(
    shared_ptr<PipelineLayout> layout, 
    VkShaderStageFlags stage_flags, 
    uint32_t offset, 
    const void* data, 
    size_t size
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::bind_push_constants] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    
    vkCmdPushConstants(command_buffer, *layout, stage_flags, offset, size, data);
}

void CommandBuffer::copy_buffer(
    shared_ptr<Buffer> dst_buffer, 
    shared_ptr<Buffer> src_buffer, 
    VkDeviceSize size, 
    VkDeviceSize dst_offset, 
    VkDeviceSize src_offset
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::copy_buffer] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    VkBufferCopy copy_region;
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, *src_buffer, *dst_buffer, 1, &copy_region);
}

void CommandBuffer::copy_buffer_to_image(
    shared_ptr<Image> dst_image, 
    shared_ptr<Buffer> src_buffer, 
    VkImageLayout image_layout, 
    const vector<VkBufferImageCopy> regions
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::copy_buffer_to_image] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    
    vkCmdCopyBufferToImage(command_buffer, 
        *src_buffer,
        *dst_image, 
        image_layout, 
        static_cast<uint32_t>(regions.size()),
        regions.data()
    );
}

void CommandBuffer::copy_image(shared_ptr<Image> dst_image, 
        shared_ptr<Image> src_image, 
        VkImageLayout dst_image_layout,
        VkImageLayout src_image_layout,
        uint32_t region_count,
        const vector<VkImageCopy> regions
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::copy_image] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    
    vkCmdCopyImage(command_buffer, 
        *src_image, src_image_layout, 
        *dst_image, dst_image_layout, 
        static_cast<uint32_t>(regions.size()), 
        regions.data()
    );
}

void CommandBuffer::copy_image_to_buffer(
    shared_ptr<Image> src_image, 
    shared_ptr<Buffer> dst_buffer, 
    VkImageLayout src_image_layout, 
    const vector<VkBufferImageCopy>& regions
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::copy_image_to_buffer] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    
    vkCmdCopyImageToBuffer(command_buffer, 
        *src_image, src_image_layout, 
        *dst_buffer, 
        static_cast<uint32_t>(regions.size()), 
        regions.data()
    );
}

void CommandBuffer::draw(uint32_t vertex_count, 
    uint32_t instance_count, 
    uint32_t first_vertex, 
    uint32_t first_instance
) {
    vkCmdDraw(command_buffer, 
        vertex_count, 
        instance_count, 
        first_vertex, 
        first_instance
    );
}

void CommandBuffer::draw_indexed(uint32_t index_count, 
    uint32_t instance_count, 
    uint32_t first_index, 
    int32_t vertex_offset, 
    uint32_t first_instance) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::draw_indexed] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    vkCmdDrawIndexed(command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::set_viewport(
    float x,
    float y,
    float width, 
    float height, 
    float min_depth, 
    float max_depth
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::set_viewport] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    VkViewport viewport = {};
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = min_depth;
    viewport.maxDepth = max_depth;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
}

void CommandBuffer::set_viewports(
    const vector<VkViewport> viewports
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::set_viewports] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    vkCmdSetViewport(command_buffer, 0, static_cast<uint32_t>(viewports.size()), viewports.data());
}

void CommandBuffer::set_scissor(
    uint32_t offset_x, 
    uint32_t offset_y, 
    uint32_t width, 
    uint32_t height
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::set_scissor] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    VkRect2D scissor = {};
    scissor.offset.x = offset_x;
    scissor.offset.y = offset_y;
    scissor.extent.width = width;
    scissor.extent.height = height;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void CommandBuffer::set_scissors(
    const vector<VkRect2D> scissors
) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::set_scissors] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    vkCmdSetScissor(command_buffer, 0, static_cast<uint32_t>(scissors.size()), scissors.data());
}

void CommandBuffer::dispatch(uint32_t group_count_x, 
    uint32_t group_count_y, 
    uint32_t group_count_z) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::dispatch] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }
    vkCmdDispatch(command_buffer, group_count_x, group_count_y, group_count_z);
}

VkResult CommandBuffer::pipeline_barrier(
    VkPipelineStageFlags src_stage_mask,
    VkPipelineStageFlags dst_stage_mask,
    const vector<ev::ImageMemoryBarrier> image_memory_barriers,
    const vector<ev::BufferMemoryBarrier> buffer_memory_barriers,
    const vector<ev::MemoryBarrier> memory_barriers
)  {
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


VkResult CommandBuffer::begin(VkCommandBufferUsageFlags flags) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::begin] : Command buffer is not allocated.");
        return VK_SUCCESS;
    }
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = flags;
    return vkBeginCommandBuffer(command_buffer, &begin_info);
}

void CommandBuffer::bind_compute_pipeline(std::shared_ptr<ComputePipeline> &pipeline) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::bind_compute_pipeline] : Command buffer is not allocated.");
        exit(EXIT_FAILURE);
    }

    if (!pipeline) {
        ev_log_error("[CommandBuffer::bind_compute_pipeline] : Invalid compute pipeline provided.");
        exit(EXIT_FAILURE);
    }

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
}

VkResult CommandBuffer::reset(VkCommandBufferResetFlags flags) {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::reset] : Command buffer is not allocated.");
        return VK_SUCCESS;
    }
    return vkResetCommandBuffer(command_buffer, flags);
}

VkResult CommandBuffer::end() {
    if (command_buffer == VK_NULL_HANDLE) {
        ev_log_error("[CommandBuffer::end] : Command buffer is not allocated.");
        return VK_SUCCESS;
    }

    return vkEndCommandBuffer(command_buffer);
}

void CommandBuffer::destroy() {
    if (command_buffer != VK_NULL_HANDLE) {
        ev_log_info("[CommandBuffer::destroy] : Destroying CommandBuffer.");
        vkFreeCommandBuffers(*device, command_pool, 1, &command_buffer);
        command_buffer = VK_NULL_HANDLE;
        ev_log_info("[CommandBuffer::destroy] : CommandBuffer destroyed successfully.");
    }
}

CommandBuffer::~CommandBuffer() {
    destroy();
}