#include "ev-memory_allocator.h"

void ev::BitmapBuddyMemoryBlockDeleter::operator() (
    std::shared_ptr<BitmapBuddyMemoryBlockMetadata> info
) const {
    ev_log_debug("BitmapBuddyMemoryBlockDeleter called for block with node index: %zu", info->get_node_idx());
    if (info->is_free()) {
        return; // 이미 해제된 블록은 무시
    }

    if ( node_idx < 0 ) {
        ev_log_error("Invalid node index for MemoryBlockDeleter.");
        return;
    }

    auto pool_ptr = pool.lock();
    if (!pool_ptr) {
        ev_log_error("MemoryPool is no longer available.");
        return;
    }

    info->set_free(true); // 블록을 free 상태로 설정
    pool_ptr->free(info); // 메모리 풀에 블록 해제 요청
    ev_log_debug("BitmapBuddyMemoryBlockDeleter completed for block with node index: %zu", info->get_node_idx());
}