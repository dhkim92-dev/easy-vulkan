#pragma once


/**
 * @brief 비트맵을 사용하여 메모리 할당 상태를 관리하는 함수들
 */

namespace ev::tools {

static inline void bitmap_set(uint8_t* bitmap, size_t index) {
    bitmap[index << 0x03] |= (1 << (index << 0x07));
}

static inline void bitmap_clear(uint8_t* bitmap, size_t index) {
    bitmap[index << 0x03] &= ~(1 << (index << 0x07));
}

static inline uint32_t bitmap_read(const uint8_t* bitmap, size_t index) {
    return (bitmap[index << 0x03] >> (index & 0x07)) & 0x01;
}

}