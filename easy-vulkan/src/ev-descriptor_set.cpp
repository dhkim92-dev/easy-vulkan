#include "ev-descriptor_set.h"

using namespace std;
using namespace ev;

DescriptorSetLayout::DescriptorSetLayout(shared_ptr<Device> _device, VkDescriptorSetLayoutCreateFlags _flags)
    : device(std::move(_device)) {
    if (!device) {
        ev_log_error("[ev::DescriptorSetLayout] Invalid device provided for DescriptorSetLayout creation.");
        exit(EXIT_FAILURE);
    }
}

void DescriptorSetLayout::add_binding(VkShaderStageFlags flags, 
    VkDescriptorType type, 
    uint32_t binding, 
    uint32_t count) {

    ev_log_debug("[ev::DescriptorSetLayout] Adding binding: %d, type: %d, count: %d, flags: %d", static_cast<int>(binding), static_cast<int>(type), static_cast<int>(count), static_cast<int>(flags));

    if ( layout != VK_NULL_HANDLE ) {
        ev_log_warn("[ev::DescriptorSetLayout] DescriptorSetLayout already created. can not add new binding.");
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
            ev_log_warn("[ev::DescriptorSetLayout] Binding %d already exists, updating descriptor count.", static_cast<int>(binding));
            return;
        }
    }
    bindings.push_back(layout_binding);
    ev_log_debug("[ev::DescriptorSetLayout] Binding added: %d, type: %d, count: %d, flags: %d", static_cast<int>(binding), static_cast<int>(type), static_cast<int>(count), static_cast<int>(flags));
}

VkResult DescriptorSetLayout::create_layout() {
    if (layout != VK_NULL_HANDLE) {
        ev_log_warn("[ev::DescriptorSetLayout] DescriptorSetLayout already created, destroying the old layout.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (bindings.empty()) {
        ev_log_error("[ev::DescriptorSetLayout] No bindings added to the DescriptorSetLayout.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();
    layout_info.flags = flags;

    VkResult result = vkCreateDescriptorSetLayout(*device, &layout_info, nullptr, &layout);
    if (result != VK_SUCCESS) {
        ev_log_error("[ev::DescriptorSetLayout] Failed to create DescriptorSetLayout: %d", static_cast<int>(result));
        return result;
    }
    ev_log_debug("[ev::DescriptorSetLayout] DescriptorSetLayout created successfully with handle: %p", reinterpret_cast<void*>(layout));
    return VK_SUCCESS;
}

void DescriptorSetLayout::destroy() {
    ev_log_info("[ev::DescriptorSetLayout] Destroying DescriptorSetLayout.");
    if (layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(*device, layout, nullptr);
        layout = VK_NULL_HANDLE;
    }
    ev_log_info("[ev::DescriptorSetLayout] DescriptorSetLayout destroyed successfully.");
}

DescriptorSetLayout::~DescriptorSetLayout() {
    destroy();
}

DescriptorSet::DescriptorSet(shared_ptr<Device> _device, VkDescriptorSet _descriptor_set)
    : device(std::move(_device)),  descriptor_set(_descriptor_set) {
    if (!device) {
        ev_log_error("[ev::DescriptorSet] Invalid device provided for DescriptorSet creation.");
        exit(EXIT_FAILURE);
    }

    if (descriptor_set == VK_NULL_HANDLE) {
        ev_log_error("[ev::DescriptorSet] Invalid descriptor set provided for DescriptorSet creation.");
        exit(EXIT_FAILURE);
    }
}

void DescriptorSet::write_buffer(uint32_t binding,
    shared_ptr<Buffer> buffer,
    VkDescriptorType type
) {
    if (!buffer) {
        ev_log_error("[ev::DescriptorSet::write_buffer] Invalid buffer provided for DescriptorSet write.");
        return;
    }
    buffer_infos.emplace_back(buffer->get_descriptor());
    WriteInfo write_info = {binding, type, static_cast<uint32_t>(buffer_infos.size() - 1)};
    buffer_write_infos.emplace_back(write_info);
    ev_log_debug("[ev::DescriptorSet::write_buffer] Buffer info added for binding: %p, offset: %llu, range: %llu", 
        reinterpret_cast<void*>(buffer->get_descriptor().buffer),
        static_cast<unsigned long long>(buffer->get_descriptor().offset),
        static_cast<unsigned long long>(buffer->get_descriptor().range)
    );
    ev_log_debug("[ev::DescriptorSet::write_buffer] Writing buffer to descriptor set with binding: %d, type: %d", static_cast<int>(binding), static_cast<int>(type));
}

void DescriptorSet::write_texture(uint32_t binding, 
    shared_ptr<ev::Texture> texture,
    VkDescriptorType type
) {
    if (!texture->image || !texture->image_view || !texture->sampler) {
        ev_log_error("[ev::DescriptorSet::write_texture] Invalid image, view, or sampler provided for DescriptorSet texture write.");
        return;
    }

    image_infos.emplace_back( texture->get_descriptor() );
    WriteInfo write_info = {binding, type, static_cast<uint32_t>(image_infos.size() - 1)};
    image_write_infos.emplace_back(write_info);
    ev_log_debug("[ev::DescriptorSet::write_texture] Writing texture to descriptor set with binding: %d, type: %d", static_cast<int>(binding), static_cast<int>(type));
}

void DescriptorSet::write_texture(uint32_t binding, 
    shared_ptr<ev::Texture> texture,
    VkDescriptorType type,
    VkImageLayout layout
) {
    if (!texture->image || !texture->image_view || !texture->sampler) {
        ev_log_error("[ev::DescriptorSet::write_texture] Invalid image, view, or sampler provided for DescriptorSet texture write.");
        return;
    }

    auto bind_descriptor = texture->get_descriptor();
    bind_descriptor.imageLayout = layout;

    image_infos.emplace_back( bind_descriptor );
    WriteInfo write_info = {binding, type, static_cast<uint32_t>(image_infos.size() - 1)};
    image_write_infos.emplace_back(write_info);

    ev_log_debug("[ev::DescriptorSet::write_texture] Writing texture to descriptor set with binding: %d, type: %d", static_cast<int>(binding), static_cast<int>(type));
}

VkResult DescriptorSet::update() {
    ev_log_debug("[ev::DescriptorSet::update] Updating DescriptorSet with %d buffer writes and %d image writes.", static_cast<int>(buffer_write_infos.size()), static_cast<int>(image_write_infos.size()));

    for ( const auto& buffer_write_info : buffer_write_infos ) {
        VkWriteDescriptorSet write_set = {};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = descriptor_set;
        write_set.dstBinding = buffer_write_info.binding;
        write_set.descriptorCount = 1; // Assuming one buffer per binding
        write_set.descriptorType = buffer_write_info.type;
        write_set.pBufferInfo = &buffer_infos[buffer_write_info.resource_index];
        write_registry.emplace_back(write_set);

        ev_log_debug("[ev::DescriptorSet::update] Writing to descriptor set with binding: %d, type: %d, count: %d, buffer info: %p",    
             static_cast<int>(write_set.dstBinding),
             static_cast<int>(write_set.descriptorType),
             static_cast<int>(write_set.descriptorCount),
             reinterpret_cast<void*>(write_set.pBufferInfo->buffer)
        );
    }

    for ( const auto& image_write_info : image_write_infos ) {
        VkWriteDescriptorSet write_set = {};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = descriptor_set;
        write_set.dstBinding = image_write_info.binding;
        write_set.descriptorCount = 1; // Assuming one image per binding
        write_set.descriptorType = image_write_info.type;
        write_set.pImageInfo = &image_infos[image_write_info.resource_index];
        write_registry.emplace_back(write_set);
        // ev_log_debug("[ev::DescriptorSet::update] Writing to descriptor set with binding: " 
        //     + std::to_string(write_set.dstBinding) 
        //     + ", type: " + std::to_string(write_set.descriptorType)
        //     + ", count: " + std::to_string(write_set.descriptorCount)
        //     + ", image info: " + (write_set.pImageInfo ? std::to_string(reinterpret_cast<uintptr_t>(write_set.pImageInfo)) : "null")
        // );
    }

    // for ( const auto& write : write_registry ) {
    //     ev_log_debug("[ev::DescriptorSet::update] Writing to descriptor set with binding: " 
    //         + std::to_string(write.dstBinding) 
    //         + ", type: " + std::to_string(write.descriptorType)
    //         + ", count: " + std::to_string(write.descriptorCount)
    //         + ", image info: " + (write.pImageInfo ? std::to_string(reinterpret_cast<uintptr_t>(write.pImageInfo)) : "null") 
    //         + ", buffer info: " + (write.pBufferInfo ? std::to_string(reinterpret_cast<uintptr_t>(write.pBufferInfo->buffer)) : "null")
    //     );
    // }

    if (  write_registry.empty() ) {
        ev_log_warn("[ev::DescriptorSet::update] No valid writes to flush to DescriptorSet.");
        return VK_SUCCESS; // Nothing to do
    }

    vkUpdateDescriptorSets(*device, static_cast<uint32_t>(write_registry.size()), write_registry.data(), 0, nullptr);
    ev_log_debug("[ev::DescriptorSet::update] Flushed %d writes to DescriptorSet.", static_cast<int>(write_registry.size()));
    write_registry.clear(); // Clear the registry after flushing
    image_infos.clear();
    buffer_write_infos.clear();
    buffer_infos.clear();
    image_write_infos.clear();
    return VK_SUCCESS;
}

// void DescriptorSet::write_image(uint32_t binding, 
//     shared_ptr<ev::ImageView> image_view,
//     VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE // Default to storage image type
// ) {
//     if (!image_view || !image_view->get_image()) {
//         ev_log_error("[ev::DescriptorSet::write_image] Invalid image view or image provided for DescriptorSet image write.");
//         return;
//     }
//     WriteInfo write_info = {
//         .binding = binding,
//         .type = type,
//         .resource_index = static_cast<uint32_t>(image_infos.size())
//     };
//     image_write_infos.emplace_back(write_info);
//     // write_registry.emplace_back(write_set);
//     ev_log_debug("[ev::DescriptorSet::write_image] Writing image to descriptor set with binding: " 
//         + std::to_string(binding) 
//         + ", type: " + std::to_string(type)
//     );
// }