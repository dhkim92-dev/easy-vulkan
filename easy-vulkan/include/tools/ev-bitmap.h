#pragma once

#include "ev-logger.h"
#include <vector>

/**
 * @brief 비트맵을 사용하여 메모리 할당 상태를 관리하는 함수들
 */

namespace ev::tools {

static inline void bitmap_set(std::vector<uint8_t>& bitmap, int64_t index) {
    bitmap[index >> 0x03] |= (1 << (index & 0x07));
}

static inline void bitmap_clear(std::vector<uint8_t>& bitmap, int64_t index) {
    bitmap[index >> 0x03] &= ~(1 << (index & 0x07));
}

static inline uint8_t bitmap_read(const std::vector<uint8_t>& bitmap, int64_t index) {
    return (bitmap[index >> 0x03] & (1 << (index & 0x07))) != 0 ? 1 : 0;
}

}