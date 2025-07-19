#pragma once

#include <memory>
#include <vector>
#include "ev-device.h"
#include "ev-buffer.h"
#include "ev-image.h"


namespace ev {

class MemoryBarrier {
private:

    VkMemoryBarrier memory_barrier = {};

public:

    explicit MemoryBarrier(VkAccessFlags src_access_mask,
        VkAccessFlags dst_access_mask,
        void* next = nullptr
    );

    MemoryBarrier(const MemoryBarrier&) = delete;

    MemoryBarrier& operator=(const MemoryBarrier&) = delete;

    ~MemoryBarrier() = default;

    operator VkMemoryBarrier() const {
        return memory_barrier;
    }
};

class BufferMemoryBarrier {

private:

    VkBufferMemoryBarrier buffer_memory_barrier = {};

public:

    explicit BufferMemoryBarrier(std::shared_ptr<ev::Buffer> buffer,
        VkAccessFlags src_access_mask,
        VkAccessFlags dst_access_mask,
        VkDeviceSize size = VK_WHOLE_SIZE,
        uint32_t src_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
        uint32_t dst_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
        void* next = nullptr
    );

    BufferMemoryBarrier(const BufferMemoryBarrier&) = delete;
 
    BufferMemoryBarrier& operator=(const BufferMemoryBarrier&) = delete;
 
   ~BufferMemoryBarrier() = default;

    operator VkBufferMemoryBarrier() const {
        return buffer_memory_barrier;
    }
};

/**
 * @brief VkImageMemoryBarrier 래퍼 클래스
 * @param image 이미지 객체
 * @param src_access_mask 소스 접근 마스크
 * @param dst_access_mask 대상 접근 마스크
 * @param old_layout 이전 이미지 레이아웃
 * @param new_layout 새로운 이미지 레이아웃
 * @param src_queue_family_index 소스 큐 패밀리 인덱스 (기본값: VK_QUEUE_FAMILY_IGNORED)
 * @param dst_queue_family_index 대상 큐 패밀리 인덱스 (기본값: VK_QUEUE_FAMILY_IGNORED)
 * @param subresource_range 서브리소스 범위 (기본값: {})
 */
class ImageMemoryBarrier {
private:

    VkImageMemoryBarrier image_memory_barrier = {};

public: 
    explicit ImageMemoryBarrier(std::shared_ptr<ev::Image> image,
        VkAccessFlags src_access_mask, 
        VkAccessFlags dst_access_mask, 
        VkImageLayout old_layout,
        VkImageLayout new_layout, 
        uint32_t src_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
        uint32_t dst_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
        VkImageSubresourceRange subresource_range = {}
    );

    ImageMemoryBarrier(const ImageMemoryBarrier&) = default;

    ImageMemoryBarrier& operator=(const ImageMemoryBarrier&) = default;

    ~ImageMemoryBarrier() = default;

    operator VkImageMemoryBarrier() const {
        return image_memory_barrier;
    }
};

class Semaphore {

private:

    std::shared_ptr<ev::Device> device;

    VkSemaphore semaphore = VK_NULL_HANDLE;

    public :

    explicit Semaphore(std::shared_ptr<ev::Device> device, 
        VkSemaphoreCreateFlags flags = 0,
        void* next = nullptr);

    Semaphore(const Semaphore&) = delete;

    Semaphore& operator=(const Semaphore&) = delete;

    void destroy();

    ~Semaphore();

    operator VkSemaphore() const {
        return semaphore;
    }
};

class Fence {

private:

    std::shared_ptr<ev::Device> device;

    VkFence fence = VK_NULL_HANDLE;

public :

    explicit Fence(std::shared_ptr<ev::Device> device, VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT, void* next = nullptr);

    Fence(const Fence&) = delete;

    Fence& operator=(const Fence&) = delete;

    VkResult wait(uint64_t timeout = UINT64_MAX);

    VkResult reset();

    void destroy();

    ~Fence();

    operator VkFence() const {
        return fence;
    }   
};

class Event {

private:

    std::shared_ptr<ev::Device> device;

    VkEvent event = VK_NULL_HANDLE;

public:

    explicit Event(std::shared_ptr<ev::Device> device, VkEventCreateFlags flags = 0, void* next = nullptr);

    Event(const Event&) = delete;

    Event& operator=(const Event&) = delete;

    void destroy();

    ~Event();

    operator VkEvent() const {
        return event;
    }
};

}