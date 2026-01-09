#include "ev-memory_allocator.h"

using namespace ev;

MemoryPool::MemoryPool(
    std::shared_ptr<ev::Device> device, 
    uint32_t memory_type_index
) : device(std::move(device)), memory_type_index(memory_type_index) {
    ev_log_info("[ev::MemoryPool] constructor called.");
    if (!this->device) {
        ev_log_error("[ev::MemoryPool] Invalid device provided for MemoryPool creation.");
        exit(EXIT_FAILURE);
    }
}

MemoryPool::~MemoryPool() {
    if ( is_initialized.load() && memory != nullptr) {
        ev_log_debug("[ev::MemoryPool] Destroying MemoryPool...");
        mbt.bitmap.clear();
        // memory->destroy();
        memory.reset();
        memory = nullptr;
        is_initialized.store(false);
    } else {
        ev_log_debug("[ev::MemoryPool] MemoryPool was not initialized or memory is null, skipping destruction.");
    }
}

VkResult MemoryPool::create(VkDeviceSize size, int32_t min_order) {
    if (is_initialized.load()) {
        ev_log_warn("[ev::MemoryPool] MemoryPool is already initialized, skipping create.");
        return VK_SUCCESS;
    }

    if (size == 0) {
        ev_log_error("[ev::MemoryPool] MemoryPool size must be greater than 0.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    this->mbt.min_order = min_order;
    this->mbt.max_order = get_max_order(size);
    this->mbt.min_blk_size = 1UL << mbt.min_order; 
    this->size = 1UL << mbt.max_order; // 메모리 풀의 최대 크기

    if ( size < mbt.min_blk_size ) {
        ev_log_error("[ev::MemoryPool] MemoryPool size must be at least %llu bytes.", static_cast<unsigned long long>(mbt.min_blk_size));
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    ev_log_debug(
        "[ev::MemoryPool] Creating MemoryPool with size: %llu, min_order: %d, max_order: %d",
        static_cast<unsigned long long>(size),
        mbt.min_order,
        mbt.max_order
    );

    // this->mbt.level = mbt.max_order - mbt.min_order + 1; // 트리의 레벨 수
    this->size = this->mbt.max_blk_size;
    // this->size = 1UL << mbt.max_order; // 메모리 풀의 최대 크기 
    memory = std::make_shared<ev::Memory>(
        device,
        memory_type_index,
        this->size
    );
    // VkResult result = memory->allocate();
    VkResult result = memory->allocate();

    if (result != VK_SUCCESS) {
        ev_log_error("[ev::MemoryPool] Failed to allocate memory for MemoryPool.");
        return result;
    }
    this->size = size;

    init_memory_block_tree();
    ev_log_info(
        "[ev::MemoryPool] Creating MemoryPool with size: %llu, min_order: %d, max_order: %d, min_blk_size: %llu, max_blk_size: %llu, level: %d, bitmap_size: %zu",
        static_cast<unsigned long long>(this->size),
        mbt.min_order,
        mbt.max_order,
        static_cast<unsigned long long>(mbt.min_blk_size),
        static_cast<unsigned long long>(mbt.max_blk_size),
        mbt.level,
        mbt.bitmap_size
    );
    return result;
}


int32_t MemoryPool::get_max_order(VkDeviceSize size) {
    int32_t order = mbt.min_order;
    VkDeviceSize block_size = 1UL << mbt.min_order; // 최소 블록 크기

    while (block_size < size ) {
        order++;
        block_size <<= 1; // 블록 크기를 두 배로 증가
    }

    return order;
}

void MemoryPool::init_memory_block_tree() {
    if (is_initialized.load()) {
        ev_log_warn("[ev::MemoryPool] MemoryBlockTree is already initialized, skipping init.");
        return;
    }
    // 트리의 최대 높이 계산, 전체 크기는 2^max_order, 최소 블록의 크기는 2^min_order,
    mbt.level = mbt.max_order - mbt.min_order + 1; // 트리의 레벨 수
    mbt.bitmap_size = ((1UL << (mbt.level)) - 1 ) / 8 + 1; // 비트맵 크기 계산 uint8_t 단위
    mbt.bitmap.resize(mbt.bitmap_size, 0x00); // 비트맵 초기화, 모든 비트가 1로 설정 (free 상태)
    mbt.bitmap[0] = 0x01;
    mbt.max_blk_size = 1UL << mbt.max_order; // 최대 블록 크기, 2^max_order 바이트
    mbt.node_count = (1UL << mbt.level) - 1; // 트리의 노드 수, 2^level - 1

    // ev_log_debug("MemoryBlockTree height : " + std::to_string(mbt.level) + 
    //     ", bitmap size: " + std::to_string(mbt.bitmap_size) 
    // );

    is_initialized.store(true);
    ev_log_debug("[ev::MemoryPool] MemoryBlockTree initialized successfully.");
}


size_t MemoryPool::translate_addr_offset_from_node(
    int32_t level,
    size_t node_idx
) {
    // ev::tools::bitmap_clear(mbt.bitmap, node_idx); // 현재 노드 비트를 할당상태로 변경
    // node_index - level_offset(level) = 현재 레벨에서 노드의 오프셋
    // 0 부터 level - 1 까지 모든
    size_t local_offset = node_idx - MemoryPool::level_offset(level); // 노드 오프셋 계산
    size_t level_blk_size = (mbt.max_blk_size >> level); // 현재 레벨의 블록 크기 계산
    return local_offset * level_blk_size;
}

int32_t MemoryPool::find_free_node(int32_t level, size_t node_idx, int32_t target_level, uint32_t alignment) {
    // ev_log_debug(
    //     "Searching for free node at level: " + std::to_string(level) + 
    //     ", node_idx: " + std::to_string(node_idx) + 
    //     ", target_level: " + std::to_string(target_level) + 
    //     ", block size : " + std::to_string(mbt.max_blk_size >> level)
    // );

    if ( node_idx >= mbt.node_count ) {
        ev_log_error("[ev::MemoryPool] Node index out of bounds: %zu", node_idx);
        return -1; // 노드 인덱스가 비트맵 범위를 벗어남
    }

    size_t blk_size = (mbt.max_blk_size >> level); // 현재 레벨의 블록 크기 계산
    size_t target_blk_size = (mbt.max_blk_size >> target_level); // 목표 레벨의 블록 크기 계산
    uint8_t is_free = ev::tools::bitmap_read(mbt.bitmap, node_idx);
    size_t offset = translate_addr_offset_from_node(level, node_idx); // 현재 노드의 오프셋 계산

    if ( blk_size < target_blk_size || (offset % alignment != 0) ) {
        return -1;
    }

    if ( level == target_level ) {
        if ( is_free ) {
            ev::tools::bitmap_clear(mbt.bitmap, node_idx);
            return node_idx; // 현재 노드가 free 상태이면, 해당 노드를 반환
        }
        return -1;
    }

    if ( level >= mbt.level - 1 ) {
        return -1;
    }

    size_t left = (node_idx << 1) + 1; // 왼쪽 자식 노드 인덱스
    size_t right = left + 1; // 오른쪽 자식 노드 인덱스

    if ( is_free ) {
        ev::tools::bitmap_set(mbt.bitmap, left); // 현재 노드 비트 설정
        ev::tools::bitmap_set(mbt.bitmap, right); // 현재 노드 비트 설정
    }

    size_t found_index = find_free_node(level + 1, left, target_level, alignment);
    if ( found_index != -1 ) {
        if ( is_free ) {
            ev::tools::bitmap_clear(mbt.bitmap, node_idx); // 현재 노드 비트 설정
            // ev::tools::bitmap_clear(mbt.bitmap, found_index); // 오른쪽 자식 노드 비트 설정
        } 
        return found_index; // 왼쪽 자식 노드에서 free 노드를 찾음
    }
    found_index = find_free_node(level + 1, right, target_level, alignment);
    if ( found_index != -1 ) {
        if ( is_free ) {
            ev::tools::bitmap_clear(mbt.bitmap, node_idx); // 현재 노드 비트 설정
            // ev::tools::bitmap_clear(mbt.bitmap, found_index);
        }
        return found_index; // 오른쪽 자식 노드에서 free 노드를 찾음
    }

    if ( is_free ) {
        ev::tools::bitmap_clear(mbt.bitmap, left); // 현재 노드 비트 설정
        ev::tools::bitmap_clear(mbt.bitmap, right); // 현재 노드 비트 설정
    }

    return -1;
}

std::shared_ptr<ev::MemoryBlockMetadata> MemoryPool::allocate_internal(
    VkDeviceSize size, 
    uint32_t alignment
) {
    int32_t target_level = 0;
    size_t block_size = mbt.max_blk_size; // 가장 큰 블록에서부터 탐색을 시작

    ev_log_debug(
        "[ev::MemoryPool] Request memory block: size = %llu, alignment = %u",
        static_cast<unsigned long long>(size),
        alignment
    );

    if ( size > mbt.max_blk_size ) {
        ev_log_error("[ev::MemoryPool] Requested size exceeds maximum block size.");
        return nullptr; // 요청한 크기가 최대 블록 크기를 초과함
    }

    // 요청한 크기가 최소 블록 크기보다 작으면 최소 블록 크기로 설정
    size_t min_size = mbt.min_blk_size > size ? mbt.min_blk_size : size; 

    while ( block_size > min_size && target_level < mbt.level - 1 ) {
        block_size >>= 1; // 블록의 크기를 반으로 줄임.
        if(block_size < size) {
            // 다음 블록 사이즈가 할당 요청 크기보다 작으면
            // 현재 블록 사이즈를 유지하고 탐색을 종료
            block_size <<= 1;
            break;
        }
        target_level++;   // 트리의 단계를 1 증가 시킴
    }
        
    // ev_log_debug(
    //     "Allocating request memory block: size = " + std::to_string(size) + 
    //     ", alignment = " + std::to_string(alignment) + 
    //     ", target_level = " + std::to_string(target_level) +
    //     ", block_size = " + std::to_string(block_size)
    // );

    // 이 과정까지 왔다면, 탐색의 대상 되는 노드들은  기본적으로 크기를 만족한다.
    // 따라서 alignment 만 고려하면 된다.
    int32_t node = find_free_node(0, 0, target_level, alignment);
    if ( node < 0 ) {
        ev_log_error("[ev::MemoryPool] No free memory block found for the requested size.");
        return nullptr; // 할당할 수 있는 블록이 없음
    }

    std::shared_ptr<ev::MemoryBlockMetadata> blk_info = 
        std::make_shared<ev::BitmapBuddyMemoryBlockMetadata>(
            memory, // memory 객체
            memory_type_index, // 메모리 타입 인덱스
            node, // 할당된 노드 인덱스
            translate_addr_offset_from_node(target_level, node), // VkDeviceMemory 의 시작 오프셋
            block_size, // 할당된 메모리 크기
            false // 독립적 할당 아님
        );

    ev_log_debug("[ev::MemoryPool] Allocated memory block: %s", blk_info->to_string().c_str());

    return blk_info;
}

void MemoryPool::free(
    std::shared_ptr<MemoryBlockMetadata> info
) {
    if (info -> is_free()) {
        return;
    }

    auto casted = std::dynamic_pointer_cast<ev::BitmapBuddyMemoryBlockMetadata>(info);
    if ( !casted ) {
        ev_log_error("[ev::MemoryPool] Invalid MemoryBlockMetadata type for free operation.");
        return; // 잘못된 타입의 메모리 블록 정보
    }
    size_t node_idx = casted->get_node_idx();

    uint8_t is_free = ev::tools::bitmap_read(mbt.bitmap, node_idx);

    if ( is_free ) {
        ev_log_warn("[ev::MemoryPool] Memory block is already free: %s", casted->to_string().c_str());
        return; // 이미 해제된 블록
    }

    ev_log_debug("[ev::MemoryPool] Freeing memory block: %s", casted->to_string().c_str());
    size_t blk_size = casted->get_size();
    // ev::tools::bitmap_clear(mbt.bitmap, node_idx); // 현재 노드 비트 설정
    // mark_parents_and_self(mbt.bitmap, node_idx); // 부모 노드 마킹

    if ( node_idx >= mbt.node_count ) {
        return; // 노드 인덱스가 비트맵 범위를 벗어남
    }

    merge(node_idx);

    casted->set_free(true); // 메모리 블록 해제 상태로 변경
}

void MemoryPool::merge(size_t node_idx) {
    if ( node_idx >= mbt.bitmap_size * 8 ) {
        ev_log_error("[ev::MemoryPool] Node index out of bounds: %zu", node_idx);
        return; // 노드 인덱스가 비트맵 범위를 벗어남
    } 
    ev::tools::bitmap_set(mbt.bitmap, node_idx); // 현재 노드를 할당 가능 상태로 변경
    if ( node_idx == 0 ) {
        return; // 루트 노드는 부모가 없으므로 종료
    }
    
    size_t buddy_idx = (node_idx % 2 == 1) ? node_idx + 1 : node_idx - 1; // 버디 노드 인덱스 계산

    // ev_log_debug(
    //     "Merging node: " + std::to_string(node_idx) + 
    //     ", parent: " + std::to_string((node_idx - 1) >> 1) +
    //     ", buddy: " + std::to_string(buddy_idx) + 
    //     ", node_count : " + std::to_string(mbt.node_count) +
    //     ",bitmap[buddy_idx] : " + std::to_string(ev::tools::bitmap_read(mbt.bitmap, buddy_idx)) + 
    //     ",bitmap[node_idx] : " + std::to_string(ev::tools::bitmap_read(mbt.bitmap, node_idx))
    // );

    if ( buddy_idx < mbt.node_count && (ev::tools::bitmap_read(mbt.bitmap, buddy_idx)) ) {
        // 버디 노드가 free 상태이면 현재 노드와 병합하여 부모를 free로 만들고
        // 자신과  버디 노드를 할당 불가능 상태로 변경
        // ev_log_debug(
        //     "Merging with buddy node: " + std::to_string(buddy_idx)
        // );
        size_t parent_idx = (node_idx - 1) >> 1; // 부모 노드 인덱스 계산
        ev::tools::bitmap_clear(mbt.bitmap, buddy_idx); // 버디 노드 사용할 수 없는 상태
        ev::tools::bitmap_clear(mbt.bitmap, node_idx); // 현재 노드 사용할  수 없는 상태
        ev::tools::bitmap_set(mbt.bitmap, parent_idx); // 부노는 할당 가능 상태 
        merge(parent_idx); // 부모 노드로 병합
    }
}

std::shared_ptr<ev::MemoryBlockMetadata> MemoryPool::standalone_allocate(
    VkDeviceSize size, 
    uint32_t alignment
) {
    if (size < alignment) {
        size = alignment;
    }

    auto standalone_memory = std::make_shared<ev::Memory>(
        device,
        memory_type_index,
        size
    );

    VkResult result = standalone_memory->allocate();

    if (result != VK_SUCCESS) {
        ev_log_error("[ev::MemoryPool] Failed to allocate standalone memory.");
        return nullptr;
    }

    return std::make_shared<ev::BitmapBuddyMemoryBlockMetadata>(
        standalone_memory,
        memory_type_index,
        0, // 노드 인덱스는 0으로 설정
        0, // 오프셋은 0으로 설정
        size, // 할당된 메모리 크기
        true // 독립적 할당
    );
}

std::shared_ptr<ev::MemoryBlockMetadata> MemoryPool::allocate(
    VkDeviceSize size, 
    uint32_t alignment
) {
    std::shared_ptr<ev::MemoryBlockMetadata> blk_info = allocate_internal(size, alignment);
    if ( blk_info == nullptr ) {
        return standalone_allocate(size, alignment);
    }
    return blk_info;
}

void MemoryPool::print_pool_status() const {
    ev_log_debug("[ev::MemoryPool] Status:");
    ev_log_debug("[ev::MemoryPool] Memory Type Index: %u", memory_type_index);
    ev_log_debug("[ev::MemoryPool] Total Size: %llu bytes", static_cast<unsigned long long>(size));
    ev_log_debug("[ev::MemoryPool] Bitmap Size: %zu bytes", mbt.bitmap_size);
    ev_log_debug("[ev::MemoryPool] Min Block Size: %llu bytes", static_cast<unsigned long long>(mbt.min_blk_size));
    ev_log_debug("[ev::MemoryPool] Max Block Size: %llu bytes", static_cast<unsigned long long>(mbt.max_blk_size));
    // bitmap 상태 출력
    ev_log_debug("[ev::MemoryPool] ---- Bitmap Status ----");
    std::string byte_str;
    for (size_t i = 0; i < mbt.bitmap_size; ++i) {
        for (int j = 7; j >= 0; --j) {
            byte_str += (mbt.bitmap[i] & (1 << j)) ? '1' : '0';
        }
        byte_str += ' ';
    }
    ev_log_debug("[ev::MemoryPool] %s", byte_str.c_str());
}