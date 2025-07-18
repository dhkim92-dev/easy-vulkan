#pragma once 

#include <vector>
#include <list>
#include <memory>
#include <string>
#include <algorithm>
#include "ev-device.h"
#include "ev-buffer.h"
#include "ev-image.h"
#include "ev-image_view.h"
#include "ev-sampler.h"
#include "ev-texture.h"
#include "ev-logger.h"

using namespace std;

namespace ev {

/**
 * @brief VkDescriptorSetLayout Wrapper
 * 디스크립터 셋 레이아웃을 생성하고 관리합니다.
 * 이 클래스는 디스크립터 셋 레이아웃의 바인딩을 추가하고, 레이아웃을 생성하며, 소멸시킵니다.
 */
class DescriptorSetLayout {

private:

    shared_ptr<Device> device;

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;

    vector<VkDescriptorSetLayoutBinding> bindings;

    VkDescriptorSetLayoutCreateFlags flags = 0;

public:

    explicit DescriptorSetLayout(shared_ptr<Device> _device, VkDescriptorSetLayoutCreateFlags _flags = 0);

    DescriptorSetLayout(const DescriptorSetLayout&) = delete;

    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    void add_binding(VkShaderStageFlags flags, VkDescriptorType type, uint32_t binding, uint32_t count=1);

    VkResult create_layout();

    void destroy();

    ~DescriptorSetLayout();

    VkDescriptorSetLayoutCreateFlags get_flags() const {
        return flags;
    }

    const vector<VkDescriptorSetLayoutBinding>& get_bindings() const {
        return bindings;
    }

    operator VkDescriptorSetLayout() const {
        return layout;
    }
};


/**
 * @brief VkDescriptorSet Wrapper
 * 실제 생성은 DescriptorPool 에서 수행합니다.
 * DescriptorSet 은 DescriptorPool 에서 할당된 디스크립터 셋을 소유만 하고 있으며, 
 * 디스크립터 셋의 생성과 소멸은 DescriptorPool 에서 관리합니다.
 */
class DescriptorSet{

private:

    struct WriteInfo {
        uint32_t binding;
        VkDescriptorType type;
        uint32_t resource_index;
    };

    shared_ptr<Device> device;

    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

    vector<VkWriteDescriptorSet> write_registry;

    vector<VkDescriptorBufferInfo> buffer_infos;

    vector<WriteInfo> buffer_write_infos;

    vector<VkDescriptorImageInfo> image_infos;

    vector<WriteInfo> image_write_infos;

public:

    explicit DescriptorSet(
        shared_ptr<Device> _device,
        VkDescriptorSet _desrciptor_set
    );

    DescriptorSet(const DescriptorSet&) = delete;

    void write_buffer(uint32_t binding, 
        shared_ptr<Buffer> buffer,
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    );

    void write_texture(uint32_t binding, 
        shared_ptr<ev::Texture> texture,
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    ); 

    void write_texture(uint32_t binding, 
        shared_ptr<Texture> texture,
        VkDescriptorType type,
        VkImageLayout layout
    );

    const size_t registry_size() const {
        return write_registry.size();
    }

    VkResult update();

    DescriptorSet& operator=(const DescriptorSet&) = delete;

    ~DescriptorSet() = default;

    operator VkDescriptorSet() const {
        return descriptor_set;
    }
};

/**
 * @brief VkDescriptorPool Wrapper
 * 디스크립터 풀을 생성하고 관리합니다.
 * 이 클래스는 디스크립터 타입과 개수를 추가하고, 디스크립터 셋을 할당/소멸의 책임을 지고, VkDescriptorPool을 생성하고 소멸시킵니다.
 */
class DescriptorPool {

private:

    std::shared_ptr<Device> device;  // The Vulkan device associated with this descriptor pool

    VkDescriptorPool pool = VK_NULL_HANDLE;  // The Vulkan descriptor pool handle

    std::vector<VkDescriptorPoolSize> pool_sizes;  // Sizes of descriptor types in the pool

public: 

    void add(VkDescriptorType type, uint32_t count);

    VkResult create_pool(uint32_t max_sets = 100, VkDescriptorPoolCreateFlags flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    vector<shared_ptr<DescriptorSet>> allocates(vector<shared_ptr<DescriptorSetLayout>> layout);

    shared_ptr<DescriptorSet> allocate( shared_ptr<DescriptorSetLayout> layout );

    VkResult release(shared_ptr<DescriptorSet> descriptor_set);

    explicit DescriptorPool(std::shared_ptr<Device> _device);

    ~DescriptorPool();

    void destroy();

    operator VkDescriptorPool() const {
        return pool;
    }
};


}
