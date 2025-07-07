#pragma once

#include <memory>
#include <atomic>
#include <string>
#include "ev-memory.h"

namespace ev {

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
struct MemoryBlockMetadata {

    std::shared_ptr<ev::Memory> memory; // 할당된 메모리 객체

    uint32_t memory_type_index; // 메모리 타입 인덱스

    size_t node_idx; // 할당된 메모리 블럭의 시작 노드 인덱스,

    size_t offset; // 할당된 메모리 블럭의 시작 오프셋,

    size_t size; // 할당된 메모리 크기

    bool is_standalone = false; // 이 블록이 독립적으로 할당되었는지 여부

    std::atomic<bool> is_free = false; // 메모리 블록 해제 여부, 스레드 세이프를 위해 atomic 사용

    explicit MemoryBlockMetadata(
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
        return "MemoryBlockMetadata{"
            "memory_type_index: " + std::to_string(memory_type_index) +
            ", node_idx: " + std::to_string(node_idx) +
            ", offset: " + std::to_string(offset) +
            ", size: " + std::to_string(size) +
            ", is_standalone: " + (is_standalone ? "true" : "false") +
            ", is_free: " + (is_free.load() ? "true" : "false") +
            "}";
    }

};

}