#pragma once

#include "ev-device.h"
#include "ev-image.h"
#include "ev-logger.h"

namespace ev {

class ImageView {

private:

    shared_ptr<Device> device;

    shared_ptr<Image> image;

    VkImageView view;

    VkFormat view_format;

    VkImageViewType view_type;

    VkComponentMapping components;

    VkImageSubresourceRange subresource_range;

    VkImageViewType find_view_type(VkImageType image_type);

    VkResult check_format_compatibility();

    VkResult check_view_type_compatibility();

public:

    ImageView(shared_ptr<Device> device, 
        shared_ptr<Image> image,
        VkFormat format = VK_FORMAT_UNDEFINED,
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM,
        VkComponentMapping components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
        VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
    );

    ~ImageView();

    void destroy();

    VkFormat get_view_format() const {
        return view_format;
    }

    VkImageViewType get_view_type() const {
        return view_type;
    }

    VkComponentMapping get_components() const {
        return components;
    }

    operator VkImageView() const {
        return view;
    }
};

}
