#pragma once;

#include <memory>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "ev-device.h"
#include "ev-buffer.h"
#include "ev-image.h"
#include "ev-memory.h"
#include "ev-macro.h"
#include "presets/ev-types.h"
#include "tools/ev-tools.h"

namespace ev {

struct MemoryBlockAllocateInfo {

    std::shared_ptr<ev::Memory> memory; // 할당된 메모리 객체

    uint32_t memory_type_index; // 메모리 타입 인덱스

    size_t node_idx; // 할당된 메모리 블럭의 노드 인덱스

    size_t blk_cnt; // 할당된 메모리 블록의 갯수수

    size_t size; // 할당된 메모리 크기

    bool is_standalone = false; // 이 블록이 독립적으로 할당되었는지 여부

    std::atomic<bool> is_free = false; // 메모리 블록 해제 여부, 스레드 세이프를 위해 atomic 사용
};

struct MemoryBlockTree {
    
    uint8_t* bitmap = nullptr; // 메모리 블록의 할당 상태를 나타내는 비트맵, 1 할당됨, 0 해제됨.

    int32_t min_order = 6; //  최소 블록 크기, 64바이트, 4x4 float matrix 최소 크기

    int32_t max_order = 29; // 최대 블록 크기, 2^31 바이트, 512MB

    int32_t bitmap_size = 0; // 비트맵 크기

    int32_t level = 0; // 트리의 최대 레벨
};

/**
 * @brief 각 Memory Type Index에 해당하는 메모리를 관리하는 책임을 지닌 클래스
 */
class MemoryPool: public std::enable_shared_from_this<MemoryPool> {
    
    private : 

    std::shared_ptr<ev::Device> device = nullptr;

    uint32_t memory_type_index;

    VkDeviceSize size = 0;

    std::shared_ptr<ev::Memory> memory = nullptr;

    MemoryBlockTree mbt;

    std::atomic<bool> is_initialized = false;

    /**
     * @brief 메모리 블록 트리의 레벨에서 첫번째 노드의 오프셋을 반환합니다.
     */
    inline static size_t level_offset(uint32_t level) {
        return 1UL << level - 1;
    }

    /**
     * @brief 주어진 노드 인덱스에 해당하는 레벨을 반환합니다.
     */
    inline static uint32_t node_to_level(uint32_t node) {
        uint32_t level = 0;
        while ( level_offset(level + 1) <= node ) {
            level++;
        }
        return level;
    }

    /**
     * @brief 현재 노드에서부터 부모노드로 올라가며, 부모 노드의 자식이 모두 free 상태이면 
     * 부모 노드를 free 상태로 표시합니다.
     * @param bitmap 비트맵 포인터
     * @param node_idx 현재 노드 인덱스
     */
    static void mark_parents(uint8_t* bitmap, size_t node_idx) {
        while ( node_idx > 0 ) {
            size_t parent = (node_idx - 1) >> 2; // 부모 노드 인덱스 계산
            size_t left = (parent << 1) + 1;
            size_t right = left + 1;

            // 부모 노드의 자식이 모두 free 상태인지 확인
            if ( ev::tools::bitmap_read(bitmap, left) && ev::tools::bitmap_read(bitmap, right) ) {
                ev::tools::bitmap_set(bitmap, parent);
            } else {
                ev::tools::bitmap_clear(bitmap, parent);
            }
        }
    }

    /**
     * @brief 메모리 블록 트리를 초기화합니다.
     * @details 이 함수는 build 호출 시 내부에서 호출되며, 수명 주기동안 단 한번만 호출됩니다.
     */
    void init_memory_block_tree();

    /**
     * @brief 메모리 블록을 할당합니다.
     * @param size 할당할 메모리 블록의 크기
     * @param alignment 할당할 메모리 블록의 정렬 크기
     * @return std::shared_ptr<MemoryBlockAllocateInfo> 할당된 메모리 블록 정보
     * @details 이 함수는 메모리 블록을 할당하고, 
     *          메모리 블록 트리에서 해당 블록을 찾아 할당 정보를 반환합니다.
     *          만약 메모리 블록이 충분하지 않다면, 이 풀이 아닌 독립적으로 생성한 ev::Memory 객체를 사용하여 
     *          메모리 블록을 할당합니다.
     */
    shared_ptr<MemoryBlockAllocateInfo> allocate_internal(
        VkDeviceSize size, 
        uint32_t alignment
    );

    /**
     * @brief 메모리 블록을 해제합니다.
     * @param info 해제할 메모리 블록 정보
     * @details 이 함수는 메모리 블록을 해제합니다.
     */
    void free(shared_ptr<MemoryBlockAllocateInfo> info);

    public :

    explicit MemoryPool(std::shared_ptr<ev::Device> device, 
        uint32_t memory_type_index
    );

    ~MemoryPool();

    VkResult create(VkDeviceSize size);

    shared_ptr<MemoryBlockAllocateInfo> allocate(VkDeviceSize size, uint32_t alignment);

    bool is_support(uint32_t mem_type_index) const {
        return memory_type_index == mem_type_index;
    }
};

class MemoryAllocator : public std::enable_shared_from_this<MemoryAllocator> {

private:

    std::shared_ptr<ev::Device> device = nullptr;

    std::atomic<bool> is_initialized = false;

    std::vector<shared_ptr<MemoryPool>> memory_pools;

public: 

    explicit MemoryAllocator(std::shared_ptr<ev::Device> device);

    ~MemoryAllocator();

    void add_pool(
        VkMemoryPropertyFlags flags,
        VkDeviceSize size
    );

    VkResult initialize();

    /**
     * @brief 버퍼에 대한 메모리를 할당하고, 바인드합니다.
     * @param buffer 할당할 버퍼 객체
     * @return std::shared_ptr<ev::Buffer> 할당된 버퍼 객체
     */
    std::shared_ptr<ev::Buffer> allocate(shared_ptr<ev::Buffer> buffer);

    /**
     * @brief 이미지에 대한 메모리를 할당하고, 바인드합니다.
     * @param image 할당할 이미지 객체
     * @return std::shared_ptr<ev::Image> 할당된 이미지 객체
     */
    std::shared_ptr<ev::Image> allocate(shared_ptr<ev::Image> image);
};

}