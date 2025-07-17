#include "ev-sync.h"

using namespace ev;

ImageMemoryBarrier::ImageMemoryBarrier(
    std::shared_ptr<ev::Image> image,
    VkAccessFlags src_access_mask,
    VkAccessFlags dst_access_mask,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    uint32_t src_queue_family_index,
    uint32_t dst_queue_family_index,
    VkImageSubresourceRange subresource_range
)  {
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    image_memory_barrier.srcAccessMask = src_access_mask;
    image_memory_barrier.dstAccessMask = dst_access_mask;
    image_memory_barrier.oldLayout = old_layout;
    image_memory_barrier.newLayout = new_layout;
    image_memory_barrier.srcQueueFamilyIndex = src_queue_family_index;
    image_memory_barrier.dstQueueFamilyIndex = dst_queue_family_index;
    image_memory_barrier.image = *image;
    image_memory_barrier.subresourceRange = subresource_range;
}