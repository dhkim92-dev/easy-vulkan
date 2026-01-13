#include "ev-memory_allocator.h"
#include "ev-logger.h"

using namespace ev;

BitmapBuddyMemoryAllocator::BitmapBuddyMemoryAllocator(
    std::shared_ptr<ev::Device> device
) : device(std::move(device)) {
    if (!this->device) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] Invalid device provided for BitmapBuddyMemoryAllocator creation.");
        exit(EXIT_FAILURE);
    }
}

BitmapBuddyMemoryAllocator::~BitmapBuddyMemoryAllocator() {
    if (is_initialized.load() && !memory_pools.empty()) {
        ev_log_debug("[ev::BitmapBuddyMemoryAllocator] Destroying BitmapBuddyMemoryAllocator...");
        memory_pools.clear();
    } else {
        ev_log_debug("[ev::BitmapBuddyMemoryAllocator] was not initialized or no memory pools exist, skipping destruction.");
    }
}

void BitmapBuddyMemoryAllocator::add_pool(
    VkMemoryPropertyFlags flags,
    VkDeviceSize size
) {
    if (is_initialized.load()) {
        ev_log_warn("[ev::BitmapBuddyMemoryAllocator] is already initialized, skipping add_pool.");
        return;
    }

    uint32_t memory_type_index = device->get_memory_type_index(0xff, flags, nullptr);

    if (memory_type_index == UINT32_MAX) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] Failed to find suitable memory type index for the given flags.");
        return;
    }

    if (pool_sizes.find(memory_type_index) == pool_sizes.end()) {
        pool_sizes[memory_type_index].size = size;
        pool_sizes[memory_type_index].memory_type_index = memory_type_index;
        return;
    }

    pool_sizes[memory_type_index].size += size;
}

VkResult BitmapBuddyMemoryAllocator::build() {
    ev_log_debug("[ev::BitmapBuddyMemoryAllocator] Internal memory pool Building...");
    if (is_initialized.load()) {
        ev_log_warn("[ev::BitmapBuddyMemoryAllocator] Already initialized, skipping build.");
        return VK_SUCCESS;
    }

    for (const auto& [memory_type_index, pool_size] : pool_sizes) {
        auto memory_pool = std::make_shared<ev::MemoryPool>(device, memory_type_index);

        VkResult result = memory_pool->create(pool_size.size, 2); // 2 is the default min_order
        if (result != VK_SUCCESS) {
            memory_pools.clear();
            return result;
        }
        memory_pools[memory_type_index] = memory_pool;
    }

    is_initialized.store(true);
    ev_log_debug("[ev::BitmapBuddyMemoryAllocator] Built successfully.");
    return VK_SUCCESS;
}

VkResult BitmapBuddyMemoryAllocator::allocate_buffer(
    std::shared_ptr<ev::Buffer> buffer,
    VkMemoryPropertyFlags mem_flags
) {
    if (!is_initialized.load()) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] is not initialized, cannot allocate buffer.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    uint32_t memory_type_index = device->get_memory_type_index(buffer->get_memory_requirements().memoryTypeBits, mem_flags, nullptr);
    if (memory_type_index == UINT32_MAX) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] Failed to find suitable memory type index for the buffer.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    auto it = memory_pools.find(memory_type_index);
    if (it == memory_pools.end()) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] No memory pool found for the specified memory type index.");
        return VK_ERROR_OUT_OF_POOL_MEMORY;
    }
    VkMemoryRequirements requirements = buffer->get_memory_requirements();

    std::shared_ptr<ev::MemoryBlockMetadata> metadata = it->second->allocate(requirements.size, static_cast<uint32_t>(requirements.alignment));
    if (!metadata) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] Failed to allocate memory for the buffer.");
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;  
    }
    return buffer->bind_memory(metadata);
}

VkResult BitmapBuddyMemoryAllocator::allocate_image(
    std::shared_ptr<ev::Image> image,
    VkMemoryPropertyFlags mem_flags
) {
    if (!is_initialized.load()) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] is not initialized, cannot allocate image.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    uint32_t memory_type_index = device->get_memory_type_index(image->get_memory_requirements().memoryTypeBits, mem_flags, nullptr);
    if (memory_type_index == UINT32_MAX) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] Failed to find suitable memory type index for the image.");
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    auto it = memory_pools.find(memory_type_index);
    if (it == memory_pools.end()) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] No memory pool found for the specified memory type index.");
        return VK_ERROR_OUT_OF_POOL_MEMORY;
    }

    std::shared_ptr<ev::MemoryBlockMetadata> metadata = it->second->allocate(image->get_memory_requirements().size, 
        static_cast<uint32_t>(image->get_memory_requirements().alignment)
    );

    if (!metadata) {
        ev_log_error("[ev::BitmapBuddyMemoryAllocator] Failed to allocate memory for the image.");
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }
    return image->bind_memory(metadata->get_memory(), metadata->get_offset());
}