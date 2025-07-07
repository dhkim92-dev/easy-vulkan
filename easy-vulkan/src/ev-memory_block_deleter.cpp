#include "ev-memory_allocator.h"

void ev::MemoryBlockDeleter::operator() (
    std::shared_ptr<MemoryBlockAllocateInfo> info
) const {
    if (info->is_free.load()) {
        return; // 이미 해제된 블록은 무시
    }

    if ( node_idx < 0 ) {
        logger::Logger::getInstance().error("Invalid node index for MemoryBlockDeleter.");
        return;
    }

    auto pool_ptr = pool.lock();
    if (!pool_ptr) {
        logger::Logger::getInstance().error("MemoryPool is no longer available.");
        return;
    }

    info->is_free.store(true); // 블록을 free 상태로 설정
    pool_ptr->free(info); // 메모리 풀에 블록 해제 요청
}