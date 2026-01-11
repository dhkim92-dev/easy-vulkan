#include "easy-vulkan.h"
#include "test_common.h"
#include <gtest/gtest.h>

using namespace ev;

class MemoryPoolTest : public ::testing::Test {
protected:
    std::shared_ptr<Instance> instance;
    std::shared_ptr<PhysicalDevice> physical_device;
    std::shared_ptr<Device> device;
    std::shared_ptr<MemoryPool> memory_pool;

    void SetUp() override {
        create_default_test_context(instance, physical_device, device, true);
    }
};

TEST_F(MemoryPoolTest, CreateMemoryPool) {
    memory_pool = std::make_shared<MemoryPool>(device, 0);
    ASSERT_NE(memory_pool, nullptr);
    VkResult result = memory_pool->create(1024 * 1024); // 1MB
    EXPECT_EQ(result, VK_SUCCESS);
}

TEST_F(MemoryPoolTest, AllocateAndFreeMemoryBlock) {
    memory_pool = std::make_shared<MemoryPool>(device, 0);
    VkResult result = memory_pool->create(1024 * 1024); // 1MB
    ASSERT_EQ(result, VK_SUCCESS);

    auto block_info = memory_pool->allocate(256 * 1024, 256); // 256KB with 256 bytes alignment

    ASSERT_NE(block_info, nullptr);
    EXPECT_EQ(block_info->get_size(), 256 * 1024);
    memory_pool->free(block_info);
    // block_info.reset();
    EXPECT_TRUE(block_info->is_free());
    
}

TEST_F(MemoryPoolTest, FreeMemoryBlock) {
    memory_pool = std::make_shared<MemoryPool>(device, 0);
    VkResult result = memory_pool->create(1024 * 1024); // 1MB
    ASSERT_EQ(result, VK_SUCCESS);

    auto block_info =  dynamic_pointer_cast<ev::BitmapBuddyMemoryBlockMetadata>(memory_pool->allocate(256 * 1024, 256)); // 256KB with 256 bytes alignment
    ASSERT_NE(block_info, nullptr);
    memory_pool->free(block_info);
    EXPECT_TRUE(block_info->is_free());
    int32_t node_id = block_info->get_node_idx();
    block_info.reset();
    ASSERT_EQ(block_info, nullptr);
    block_info = dynamic_pointer_cast<ev::BitmapBuddyMemoryBlockMetadata>(memory_pool->allocate(256 * 1024, 256)); // 256KB with 256 bytes alignment
    ASSERT_NE(block_info, nullptr);
    EXPECT_EQ(block_info->get_node_idx(), node_id);
}

TEST_F(MemoryPoolTest, AllocateEntireMemory) {
    memory_pool = std::make_shared<MemoryPool>(device, 0);
    VkResult result = memory_pool->create(1024 * 1024); // 1MB
    ASSERT_EQ(result, VK_SUCCESS);

    auto block_info = memory_pool->allocate(1024 * 1024, 256); // Allocate entire memory with 256 bytes alignment
    ASSERT_NE(block_info, nullptr);
    EXPECT_EQ(block_info->get_size(), 1024 * 1024);
    EXPECT_FALSE(block_info->is_standalone() );
    
    memory_pool->free(block_info);
}

TEST_F(MemoryPoolTest, AllocateWithAlignment) {
    memory_pool = std::make_shared<MemoryPool>(device, 0);
    VkResult result = memory_pool->create(1024); // 1MB, 64바이트 단위로 블록을 사용
    ASSERT_EQ(result, VK_SUCCESS);

    auto block_info_512_32 = memory_pool->allocate(512, 32);// 512bytes
    ev_log_debug(
        "blkinfo_512_32 %s", block_info_512_32->to_string().c_str()
    );
    ASSERT_NE(block_info_512_32, nullptr);
    EXPECT_EQ(block_info_512_32->get_size(), 512);
    EXPECT_EQ(block_info_512_32->get_offset() % 32, 0); // Check alignment
    EXPECT_EQ(block_info_512_32->get_offset(), 0); // Check alignment

    auto block_info_27_32 = dynamic_pointer_cast<BitmapBuddyMemoryBlockMetadata>(memory_pool->allocate(27, 32)); // 32 bytes with 32 bytes alignment
    ev_log_debug(
        "blkinfo_27_32 %s", block_info_27_32->to_string().c_str()
    );
    ASSERT_NE(block_info_27_32, nullptr);
    EXPECT_EQ(block_info_27_32->get_size(), 64);
    EXPECT_EQ(block_info_27_32->get_offset() % 32, 0); // Check alignment
    EXPECT_EQ(block_info_27_32->get_offset(), 512);
    EXPECT_EQ(block_info_27_32->get_node_idx(), 23); // Check node index

    auto block_info_127_128 = memory_pool->allocate(127, 128); // 1024 bytes with 64 bytes alignment
    ev_log_debug(
        "Allocated block info:  %s", block_info_127_128->to_string().c_str()
    );      
    ASSERT_NE(block_info_127_128, nullptr);
    EXPECT_EQ(block_info_127_128->get_size(), 128);
    EXPECT_EQ(block_info_127_128->get_offset() % 128, 0);
    EXPECT_EQ(block_info_127_128->get_offset(), 640); // Check offset after previous allocations
}

TEST_F(MemoryPoolTest, RandomFreeAndReallocateTest) {

    memory_pool = std::make_shared<MemoryPool>(device, 0);
    VkResult result = memory_pool->create(1024, 6); // 1MB
    ASSERT_EQ(result, VK_SUCCESS);

    std::vector<std::shared_ptr<MemoryBlockMetadata>> allocated_blocks;

    for ( uint32_t i = 0 ; i < 16 ; ++i ) {
        auto block_info = memory_pool->allocate(64, 64); // 256 bytes with 64 bytes alignment
        ASSERT_NE(block_info, nullptr);
        EXPECT_EQ(block_info->get_size(), 64);
        allocated_blocks.push_back(block_info);
        EXPECT_FALSE(block_info->is_free());
    }

    for ( uint32_t i = 0 ; i < 4 ; ++i ) {
        // random erase
        size_t idx = rand() % allocated_blocks.size();
        auto block_info = allocated_blocks[idx];
        memory_pool->free(block_info);
        EXPECT_TRUE(block_info->is_free());
        allocated_blocks.erase(allocated_blocks.begin() + idx);
    }

    for ( uint32_t i = 0 ; i < 4 ; ++i ) {
        auto block_info = dynamic_pointer_cast<BitmapBuddyMemoryBlockMetadata>(memory_pool->allocate(64, 64)); // 64 bytes with 64 bytes alignment
        ASSERT_NE(block_info, nullptr);
        EXPECT_EQ(block_info->get_size(), 64);
        EXPECT_FALSE(block_info->is_free());
    }
}

TEST_F(MemoryPoolTest, AllocateWithInsufficientMemory) {
    memory_pool = std::make_shared<MemoryPool>(device, 0);
    VkResult result = memory_pool->create(1024 * 1024); // 1MB
    ASSERT_EQ(result, VK_SUCCESS);

    auto block_info = memory_pool->allocate(2 * 1024 * 1024, 256); // Try to allocate more than available
    EXPECT_TRUE(block_info->is_standalone());
}

TEST_F(MemoryPoolTest, ExternalFragmentationTest) {
    memory_pool = std::make_shared<MemoryPool>(device, 0);
    VkResult result = memory_pool->create(1024); // 1MB
    ASSERT_EQ(result, VK_SUCCESS);
    // memory_pool->print_pool_status();

    // 1바이트 16개 할당
    std::vector<std::shared_ptr<MemoryBlockMetadata>> allocated_blocks;
    for ( int i = 0 ; i < 16 ; ++i ) {
        auto block_info = memory_pool->allocate(1, 64); // 1 byte
        ASSERT_NE(block_info, nullptr);
        EXPECT_EQ(block_info->get_size(), 64);
        allocated_blocks.push_back(block_info);
        // memory_pool->print_pool_status();
    }

    //256바이트 할당 시도
    auto block_info = dynamic_pointer_cast<BitmapBuddyMemoryBlockMetadata>(memory_pool->allocate(256, 256)); // 256 bytes with 256 bytes alignment
    EXPECT_TRUE(block_info->is_standalone()); // 독립적으로 할당되어야 함
    EXPECT_EQ(block_info->get_size(), 256); // 할당된 크기 확인
    EXPECT_EQ(block_info->get_offset(), 0); // 할당된 오프셋 확인
    EXPECT_EQ(block_info->get_node_idx(), 0); // 할당된 노드 인덱스 확인
    block_info.reset();

    // 1바이트 블록 4개 해제
    for( int i = 0 ; i < 4 ; ++i ) {
        memory_pool -> free(allocated_blocks.back());
        allocated_blocks.pop_back();
        // memory_pool->print_pool_status();
    }

    // 256바이트 할당 시도
    block_info = dynamic_pointer_cast<BitmapBuddyMemoryBlockMetadata>(memory_pool->allocate(256, 256)); // 256 bytes with 256 bytes alignment
    EXPECT_FALSE(block_info->is_standalone()); // 독립적으로 할당되지 않아야 함
    EXPECT_EQ(block_info->get_size(), 256); // 할당된 크기 확인
    EXPECT_EQ(block_info->get_offset(), 768); // 할당된 오프셋
    EXPECT_EQ(block_info->get_node_idx(), 6); // 할당된 노드 인덱스 확인
    memory_pool->free(block_info); // 해제
}

TEST_F(MemoryPoolTest, AllocateNextNodeForAlignment) {
    // 시나리오
    // 1024 바이트, min_order 6 풀 생성
    memory_pool = std::make_shared<MemoryPool>(device, 0);
    VkResult result = memory_pool->create(1024, 6); // 1

    // 64바이트 할당
    auto block_info_64 = dynamic_pointer_cast<BitmapBuddyMemoryBlockMetadata>(memory_pool->allocate(64, 64)); // 64 bytes with 64 bytes alignment
    ASSERT_NE(block_info_64, nullptr);
    EXPECT_EQ(block_info_64->get_size(), 64);
    EXPECT_EQ(block_info_64->get_offset(), 0); // Check offset
    EXPECT_EQ(block_info_64->get_node_idx(), 15); // Check node index

    // 256 정렬로 64바이트 할당
    auto block_info_64_256 = dynamic_pointer_cast<BitmapBuddyMemoryBlockMetadata>(memory_pool->allocate(64, 256));
    ev_log_debug(
        "blkinfo_64_256 %s", block_info_64_256->to_string().c_str()
    );
    ASSERT_NE(block_info_64_256, nullptr);
    EXPECT_EQ(block_info_64_256->get_size(), 64);
    EXPECT_EQ(block_info_64_256->get_offset(), 256); // Check offset after
    EXPECT_EQ(block_info_64_256->get_node_idx(), 19); // Check node index
}