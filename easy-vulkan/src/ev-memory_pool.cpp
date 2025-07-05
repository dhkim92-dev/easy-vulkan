#include "ev-memory_allocator.h"
#include <cstring>

using namespace ev;

MemoryPool::MemoryPool(
    std::shared_ptr<ev::Device> device, 
    uint32_t memory_type_index
) : device(std::move(device)), memory_type_index(memory_type_index) {
    if (!this->device) {
        logger::Logger::getInstance().error("Invalid device provided for MemoryPool creation.");
        exit(EXIT_FAILURE);
    }
}

MemoryPool::~MemoryPool() {
    if ( is_initialized.load() && memory != nullptr) {
        logger::Logger::getInstance().debug("Destroying MemoryPool...");
        delete[] mbt.bitmap; // 비트맵 메모리 해제
        mbt.bitmap = nullptr; // 비트맵 포인터 초기화
        memory->destroy();
        memory.reset();
        memory = nullptr;
    } else {
        logger::Logger::getInstance().debug("MemoryPool was not initialized or memory is null, skipping destruction.");
    }
}

int32_t MemoryPool::get_max_order(VkDeviceSize size) {
    int32_t order = 0;
    VkDeviceSize block_size = 1UL << mbt.min_order; // 최소 블록 크기

    while (block_size < size && order < mbt.max_order) {
        order++;
        block_size <<= 1; // 블록 크기를 두 배로 증가
    }

    return order;
}

void MemoryPool::init_memory_block_tree() {
    if (is_initialized.load()) {
        logger::Logger::getInstance().warn("MemoryBlockTree is already initialized, skipping init.");
        return;
    }
    // 트리의 최대 높이 계산, 전체 크기는 2^max_order, 최소 블록의 크기는 2^min_order,
    mbt.level = mbt.max_order - mbt.min_order + 1; // 트리의 레벨 수
    mbt.bitmap_size = (1UL << mbt.max_order) >> 3; // 비트맵 크기 계산 uint8_t 단위
    mbt.bitmap = new uint8_t[mbt.bitmap_size]; // 비트맵 초기화

    // 초기 상태는 모든 블록이 free 상태로 설정
    memset(mbt.bitmap, 0xFF, mbt.bitmap_size); // 모든 비트가 1로 설정 (free 상태)

    is_initialized.store(true);
    logger::Logger::getInstance().debug("MemoryBlockTree initialized successfully.");
}

VkResult MemoryPool::create(VkDeviceSize size, int32_t min_order) {
    if (is_initialized.load()) {
        logger::Logger::getInstance().warn("MemoryPool is already initialized, skipping create.");
        return VK_SUCCESS;
    }

    if (size == 0) {
        logger::Logger::getInstance().error("MemoryPool size must be greater than 0.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    this->mbt.min_order = min_order;
    this->mbt.max_order = get_max_order(size);
    this->size = 1UL << mbt.max_order; // 메모리 풀의 최대 크기 
    init_memory_block_tree();
    
    return VK_SUCCESS;
}