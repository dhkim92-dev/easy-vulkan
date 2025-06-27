#include "ev-descriptor_set.h"

using namespace std;
using namespace ev; 


DescriptorPool::DescriptorPool(shared_ptr<Device> _device)
    : device(std::move(_device)) {
    if (!device) {
        logger::Logger::getInstance().error("Invalid device provided for DescriptorPool creation.");
        exit(EXIT_FAILURE);
    }
}

void DescriptorPool::add(VkDescriptorType type, uint32_t count) {
    for ( int i = 0 ; i < pool_sizes.size() ; ++i ) {
        if (pool_sizes[i].type == type) {
            pool_sizes[i].descriptorCount += count;
            return;
        }
    }
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = type;
    pool_size.descriptorCount = count;
    pool_sizes.push_back(pool_size);
}

VkResult DescriptorPool::create_pool(uint32_t max_sets, VkDescriptorPoolCreateFlags flags) {
    if ( pool != VK_NULL_HANDLE ) {
        logger::Logger::getInstance().warn("Descriptor pool already created, destroying the old pool.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (pool_sizes.empty()) {
        logger::Logger::getInstance().error("No descriptor types added to the pool.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = max_sets; // Arbitrary limit, adjust as needed
    pool_info.flags = flags;;

    if (flags == VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT) {
        const vector<const char*>& extensions = device->get_enabled_extensions();
        if (std::find(extensions.begin(), extensions.end(), "VK_EXT_descriptor_indexing") == extensions.end()) {
            logger::Logger::getInstance().error("VK_EXT_descriptor_indexing extension is required for VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, default flags will be used.");
        }
    }

    return vkCreateDescriptorPool(*device, &pool_info, nullptr, &pool);
}

vector<shared_ptr<DescriptorSet>> DescriptorPool::allocates(
    vector<shared_ptr<DescriptorSetLayout>> layouts
) {
    if ( pool == VK_NULL_HANDLE ) {
        logger::Logger::getInstance().error("Descriptor pool is not created, cannot allocate descriptor sets.");
        return {};
    }   

    vector<VkDescriptorSetLayout> vk_layouts(layouts.size());

    for (size_t i = 0 ; i < vk_layouts.size() ; ++i) {
        vk_layouts[i] = *layouts[i];
    };

    VkDescriptorSetAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = pool;
    ai.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    ai.pSetLayouts =  vk_layouts.data();
    if (pool_sizes.empty()) {
        logger::Logger::getInstance().error("No descriptor types added to the pool, cannot allocate descriptor set.");
        return {};
    }
    ai.pNext = nullptr;
    vector<VkDescriptorSet> vk_descriptor_sets(layouts.size());
    CHECK_RESULT(vkAllocateDescriptorSets(*device, &ai, vk_descriptor_sets.data()));

    return [&] {
        vector<shared_ptr<DescriptorSet>> sets(layouts.size());
        for (const auto& vk_descriptor_set : vk_descriptor_sets) {
            sets.push_back(make_shared<DescriptorSet>(device, vk_descriptor_set));
        }
        return sets;
    }();
}

shared_ptr<DescriptorSet> DescriptorPool::allocate(shared_ptr<DescriptorSetLayout> layout) {
    if (pool == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Descriptor pool is not created, cannot allocate descriptor set.");
        return nullptr;
    }

    if (!layout) {
        logger::Logger::getInstance().error("DescriptorSetLayout is null, cannot allocate descriptor set.");
        return nullptr;
    }

    VkDescriptorSetAllocateInfo ai = {};
    VkDescriptorSetLayout vk_layout = *layout;
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = pool;
    ai.descriptorSetCount = 1; // Allocate one descriptor set
    ai.pSetLayouts = &vk_layout;
    VkDescriptorSet descriptor_set;
    VkResult result = vkAllocateDescriptorSets(*device, &ai, &descriptor_set);
    if (result != VK_SUCCESS) {
        logger::Logger::getInstance().error("Failed to allocate descriptor set: " + std::to_string(result));
        return nullptr;
    }

    return make_shared<DescriptorSet>(device, descriptor_set);
}

VkResult DescriptorPool::release(shared_ptr<DescriptorSet> descriptor_set) {
    if (!descriptor_set) {
        logger::Logger::getInstance().error("DescriptorSet is null, cannot release.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkDescriptorSet vk_descriptor_set = *descriptor_set;
    VkResult result = vkFreeDescriptorSets(*device, pool, 1, &(vk_descriptor_set));

    if ( result == VK_SUCCESS ) {
        descriptor_set.reset();
    }  else {
        logger::Logger::getInstance().error("Failed to free descriptor set: " + std::to_string(result));
    }
    return result;
}


void DescriptorPool::destroy() {
    if (pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(*device, pool, nullptr);
        pool = VK_NULL_HANDLE;
        pool_sizes.clear();
    }
}

DescriptorPool::~DescriptorPool() {
    destroy();
}