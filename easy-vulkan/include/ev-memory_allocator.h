#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <cmath>
#include <numeric>
#include "ev-device.h"
#include "ev-buffer.h"
#include "ev-image.h"
#include "ev-memory.h"
#include "ev-macro.h"
#include "presets/ev-types.h"
#include "tools/ev-tools.h"

namespace ev {

// constexpr uint32_t FREE = 0x01;

// constexpr uint32_t ALLOCATED = 0x00;

class MemoryPool;

/**
 * @brief 메모리 블록 할당 정보를 저장하는 구조체
 * @details 이 구조체는 메모리 블록의 할당 정보를 저장하며, 
 *          메모리 풀에서 관리되는 메모리 블록의 상태를 나타냅니다.
 * @note 이 구조체는 스레드 세이프를 위해 atomic 변수를 사용합니다.
 *       메모리 블록이 독립적으로 할당되었는지 여부는 `is_standalone` 플래그로 나타냅니다.
 *       `is_free` 플래그는 메모리 블록이 해제되었는지 여부를 나타내며, 
 *       이 플래그는 스레드 세이프를 위해 atomic으로 선언되었습니다.
 *       이 구조체를 통해 메모리를 바인딩한 객체는 반드시 내부에 이 구조체를 소유하고 있어야 합니다.
 *       그렇지 않으면 메모리 블록이 해제되었을 때 풀에 반환되지 않습니다.
 */
struct MemoryBlockAllocateInfo {

    std::shared_ptr<ev::Memory> memory; // 할당된 메모리 객체

    uint32_t memory_type_index; // 메모리 타입 인덱스

    size_t node_idx; // 할당된 메모리 블럭의 시작 노드 인덱스,

    size_t offset; // 할당된 메모리 블럭의 시작 오프셋,

    size_t size; // 할당된 메모리 크기

    bool is_standalone = false; // 이 블록이 독립적으로 할당되었는지 여부

    std::atomic<bool> is_free = false; // 메모리 블록 해제 여부, 스레드 세이프를 위해 atomic 사용

    MemoryBlockAllocateInfo(
        std::shared_ptr<ev::Memory> _memory,
        uint32_t _memory_type_index,
        size_t _node_idx,
        size_t _offset,
        size_t _size,
        bool _is_standalone = false
    ) : 
        memory(std::move(_memory)), 
        memory_type_index(_memory_type_index), 
        node_idx(_node_idx), 
        offset(_offset),
        size(_size), 
        is_standalone(_is_standalone) {}

    std::string to_string() const {
        return "MemoryBlockAllocateInfo{"
            "memory_type_index: " + std::to_string(memory_type_index) +
            ", node_idx: " + std::to_string(node_idx) +
            ", offset: " + std::to_string(offset) +
            ", size: " + std::to_string(size) +
            ", is_standalone: " + (is_standalone ? "true" : "false") +
            ", is_free: " + (is_free.load() ? "true" : "false") +
            "}";
    }

};

struct MemoryBlockTree {
    
    std::vector<uint8_t> bitmap; // 메모리 블록의 할당 상태를 나타내는 비트맵, 0 할당됨, 1 해제됨.

    int32_t min_order = 6; //  최소 블록 크기, 64바이트, 4x4 float matrix 최소 크기

    int32_t max_order = 29; // 최대 블록 크기, 2^31 바이트, 512MB

    int32_t bitmap_size = 0; // 비트맵 크기, 8비트가 각각 하나의 블록을 나타냄

    int32_t node_count = 0;

    int32_t level = 0; // 트리의 최대 레벨

    size_t min_blk_size = 64; // 최소 블록 크기, 64바이트, 4x4 float matrix 최소 크기

    size_t max_blk_size = 1UL << max_order; // 최대 블록 크기, 2^29 바이트, 512MB
};

class MemoryBlockDeleter {

    std::weak_ptr<MemoryPool> pool;

    size_t node_idx;
public:
    void operator() (std::shared_ptr<MemoryBlockAllocateInfo> info) const;
};

/**
 * @brief 각 Memory Type Index에 해당하는 메모리를 관리하는 책임을 지닌 클래스
 */
class MemoryPool {
    
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
        return (1UL << level) - 1;
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
     * @brief 주어진 메모리 크기에 대해 최대 블록 크기의 트리 오더를 반환합니다.
     * @param size 메모리 크기
     * @return int32_t 최대 블록 크기의 트리 오더
     */
    int32_t get_max_order(VkDeviceSize size);

    /**
     * @brief 현재 노드에서부터 부모노드로 올라가며, 부모 노드의 자식이 모두 free 상태이면 
     * 부모 노드를 free 상태로 표시합니다.
     * @param bitmap 비트맵 포인터
     * @param node_idx 현재 노드 인덱스
     */
    void merge(size_t node_idx);

    /**
     * @brief 노드 주소를 오프셋으로 변환하여 실제 할당될 메모리 블록의 시작 주소를 반환
     */
    size_t translate_addr_offset_from_node(int32_t level,size_t node_idx);

    /**
     * @brief 메모리 블록 트리를 초기화합니다.
     * @details 이 함수는 build 호출 시 내부에서 호출되며, 수명 주기동안 단 한번만 호출됩니다.
     */
    void init_memory_block_tree();

    /**
     * @brief 재귀적으로 노드를 탐색, 블록을 분할하며 원하는 레벨의 free node를 찾는다.
     */
    int32_t find_free_node(int32_t level, size_t node_idx, int32_t target_level, uint32_t alignment);

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

public :

    /**
     * @brief 메모리 풀 생성자 
     * @param device Vulkan 디바이스 객체
     * @param memory_type_index 메모리 타입 인덱스
     * @details 이 생성자는 실질적인 풀을 관리 데이터를 생성하지않습니다. 반드시 create 메서드를 호출하세요.
     */
    explicit MemoryPool(std::shared_ptr<ev::Device> device, 
        uint32_t memory_type_index
    );

    /**
     * @brief 메모리 풀 파괴자
     */
    ~MemoryPool();

    /**
     * @brief 메모리 블록을 해제합니다.
     * @param info 해제할 메모리 블록 정보
     * @details 이 함수는 메모리 블록을 해제합니다.
     */
    void free(shared_ptr<MemoryBlockAllocateInfo> info);

    /**
     * @brief 실제 메모리 풀을 생성합니다.
     * @param size 메모리 풀의 크기
     * @param min_order 최소 블록 크기의 트리 오더, 기본값은 6 (64바이트)
     * @return VkResult VK_SUCCESS on success, or an error code on failure
     * @details 이 함수는 메모리 풀을 생성하고,
     *          메모리 블록 트리를 초기화합니다.
     *          메모리 풀의 크기는 2^max_order 바이트로 설정됩니다.
     *          min_order는 최소 블록 크기의 트리 오더로,
     *          기본값은 6 (64바이트)입니다.
     *          이 함수는 메모리 풀을 생성한 후,
     *          메모리 블록 트리를 초기화합니다.
     *          이 함수는 반드시 메모리 풀을 생성한 후에 호출되어야 합니다.
     *          메모리 풀을 생성하기 전에 반드시 device가 초기화되어 있어야 합니다.
     */
    VkResult create(VkDeviceSize size, int32_t min_order = 6);

    /**
     * @brief 메모리 블록을 할당합니다.
     * @param size 할당할 메모리 블록의 크기
     * @param alignment 할당할 메모리 블록의 정렬 크기
     * @return std::shared_ptr<MemoryBlockAllocateInfo> 할당된 메모리 블록 정보
     */
    std::shared_ptr<MemoryBlockAllocateInfo> allocate(VkDeviceSize size, uint32_t alignment);

    /**
     * @brief 독립적으로 메모리 블록을 할당합니다.
     * @param size 할당할 메모리 블록의 크기
     * @param alignment 할당할 메모리 블록의 정렬 크기
     * @return std::shared_ptr<MemoryBlockAllocateInfo> 할당된 메모리 블록 정보
     * @details 이 함수는 메모리 블록을 독립적으로 할당하고, 
     *          메모리 블록 트리에서 해당 블록을 찾아 할당 정보를 반환합니다.
     *          이 함수는 메모리 풀의 관리을 벗어나 독립적으로 메모리 블록을 할당할 때 사용됩니다.
     */
    std::shared_ptr<MemoryBlockAllocateInfo> standalone_allocate(
        VkDeviceSize size, 
        uint32_t alignment
    ); 

    bool is_support(uint32_t mem_type_index) const {
        return memory_type_index == mem_type_index;
    }

    void print_pool_status() const;
};

struct PoolSize {
    uint32_t memory_type_index;
    VkDeviceSize size;
};

// class MemoryAllocator {

// private:

//     std::shared_ptr<ev::Device> device = nullptr;

//     std::atomic<bool> is_initialized = false;

//     std::vector<shared_ptr<MemoryPool>> memory_pools;

//     std::unordered_map<uint32_t, PoolSize> pool_sizes;

// public: 

//     explicit MemoryAllocator(std::shared_ptr<ev::Device> device);

//     ~MemoryAllocator();

//     void add_pool(
//         VkMemoryPropertyFlags flags,
//         VkDeviceSize size
//     );

//     VkResult initialize();

//     /**
//      * @brief 버퍼에 대한 메모리를 할당하고, 바인드합니다.
//      * @param buffer 할당할 버퍼 객체
//      * @return std::shared_ptr<ev::Buffer> 할당된 버퍼 객체
//      */
//     std::shared_ptr<ev::Buffer> allocate(shared_ptr<ev::Buffer> buffer);

//     /**
//      * @brief 이미지에 대한 메모리를 할당하고, 바인드합니다.
//      * @param image 할당할 이미지 객체
//      * @return std::shared_ptr<ev::Image> 할당된 이미지 객체
//      */
//     std::shared_ptr<ev::Image> allocate(shared_ptr<ev::Image> image);
// };

};