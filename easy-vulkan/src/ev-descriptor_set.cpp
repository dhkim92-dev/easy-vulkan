#include "ev-descriptor_set.h"

using namespace std;
using namespace ev;

DescriptorSetLayout::DescriptorSetLayout(shared_ptr<Device> _device, VkDescriptorSetLayoutCreateFlags _flags)
    : device(std::move(_device)) {
    if (!device) {
        logger::Logger::getInstance().error("Invalid device provided for DescriptorSetLayout creation.");
        exit(EXIT_FAILURE);
    }
}

void DescriptorSetLayout::add_binding(VkShaderStageFlags flags, 
    VkDescriptorType type, 
    uint32_t binding, 
    uint32_t count) {

    if ( layout != VK_NULL_HANDLE ) {
        logger::Logger::getInstance().warn("DescriptorSetLayout already created. can not add new binding.");
        return;
    }
    
    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = binding;
    layout_binding.descriptorType = type;
    layout_binding.descriptorCount = count; 
    layout_binding.stageFlags = flags;
    layout_binding.pImmutableSamplers = nullptr;

    for (const auto& existing_binding : bindings) {
        if (existing_binding.binding == binding) {
            logger::Logger::getInstance().warn("Binding " + std::to_string(binding) + " already exists, updating descriptor count.");
            return;
        }
    }
    bindings.push_back(layout_binding);
}

VkResult DescriptorSetLayout::create_layout() {
    if (layout != VK_NULL_HANDLE) {
        logger::Logger::getInstance().warn("DescriptorSetLayout already created, destroying the old layout.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (bindings.empty()) {
        logger::Logger::getInstance().error("No bindings added to the DescriptorSetLayout.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();
    layout_info.flags = flags;

    return vkCreateDescriptorSetLayout(*device, &layout_info, nullptr, &layout);
}

void DescriptorSetLayout::destroy() {
    if (layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(*device, layout, nullptr);
        layout = VK_NULL_HANDLE;
    }
}

DescriptorSetLayout::~DescriptorSetLayout() {
    destroy();
}

DescriptorSet::DescriptorSet(shared_ptr<Device> _device, VkDescriptorSet _descriptor_set)
    : device(std::move(_device)),  descriptor_set(_descriptor_set) {
    if (!device) {
        logger::Logger::getInstance().error("Invalid device provided for DescriptorSet creation.");
        exit(EXIT_FAILURE);
    }

    if (descriptor_set == VK_NULL_HANDLE) {
        logger::Logger::getInstance().error("Invalid descriptor set provided for DescriptorSet creation.");
        exit(EXIT_FAILURE);
    }
}

void DescriptorSet::write_buffer(uint32_t binding, shared_ptr<Buffer> buffer) {
    if (!buffer) {
        logger::Logger::getInstance().error("Invalid buffer provided for DescriptorSet write.");
        return;
    }

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = *buffer;
    buffer_info.offset = 0; // Assuming the whole buffer is used
    buffer_info.range = VK_WHOLE_SIZE; // Use the whole buffer
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = descriptor_set;
    write_set.dstBinding = binding;
    // write_set.dstArrayElement = 0;
    write_set.descriptorCount = 1;
    write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Assuming uniform buffer type
    write_set.pBufferInfo = &buffer_info;
    write_registry.emplace_back(write_set);
}

VkResult DescriptorSet::update() {
    if (write_registry.empty()) {
        logger::Logger::getInstance().warn("No writes to update in DescriptorSet.");
        return VK_SUCCESS; // Nothing to do
    }

    vkUpdateDescriptorSets(*device, static_cast<uint32_t>(write_registry.size()), write_registry.data(), 0, nullptr);
    logger::Logger::getInstance().debug("Flushed " + std::to_string(write_registry.size()) + " writes to DescriptorSet.");
    write_registry.clear(); // Clear the registry after flushing
    return VK_SUCCESS;
}