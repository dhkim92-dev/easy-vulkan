#include "ev-image_view.h"

using namespace ev;


ImageView::ImageView(shared_ptr<Device> _device, 
    shared_ptr<Image> _image, 
    VkImageViewType view_type,
    VkFormat view_format,
    VkComponentMapping components,
    VkImageSubresourceRange subresource_range
): device(std::move(_device)), image(std::move(_image)), view_format(view_format), view_type(view_type) {
    ev_log_info("[ev::ImageView] Creating image view...");
    CHECK_RESULT(check_format_compatibility());
    CHECK_RESULT(check_view_type_compatibility());

    VkImageViewCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image = *image;
    ci.viewType = this->view_type; // Adjust as necessary
    ci.format = this->view_format;
    ci.components = components;
    ci.subresourceRange = subresource_range;

    CHECK_RESULT(vkCreateImageView(*device, &ci, nullptr, &view));
}

VkResult ImageView::check_format_compatibility() {
    if (view_format == VK_FORMAT_UNDEFINED) {
        view_format = image->get_format();
    }

    if (view_format == image->get_format()) {
        return VK_SUCCESS;
    }


    if ( !(image->get_flags() & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) ) {
        ev_log_error("[ev::ImageView] Image was not created with VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, but view format is different from image format.");
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }

    VkPhysicalDevice physical_device = *device->get_physical_device();
    VkImageFormatProperties format_properties;
    VkResult result = vkGetPhysicalDeviceImageFormatProperties(physical_device,
        view_format,
        image->get_type(),
        image->get_tiling(),
        image->get_image_usage_flags(),
        image->get_flags(),
        &format_properties
    );

    if (result != VK_SUCCESS) {
        ev_log_error("[ev::ImageView] vkGetPhysicalDeviceImageFormatProperties failed for the view format.");
    }

    return result;
}

VkResult ImageView::check_view_type_compatibility() {
    VkImageType image_type = image->get_type();
    uint32_t array_layers = image->get_array_layers();
    VkImageCreateFlags flags = image->get_flags();

    // If view_type is not specified, deduce a default one.
    if (view_type == VK_IMAGE_VIEW_TYPE_MAX_ENUM) {
        switch (image_type) {
            case VK_IMAGE_TYPE_1D:
                this->view_type = (array_layers > 1) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
                break;
            case VK_IMAGE_TYPE_2D:
                this->view_type = (array_layers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                break;
            case VK_IMAGE_TYPE_3D:
                this->view_type = VK_IMAGE_VIEW_TYPE_3D;
                break;
            default:
                ev_log_error("[ev::ImageView] Invalid image type for deducing view type.");
                return VK_ERROR_INITIALIZATION_FAILED;
        }
    }

    bool compatible = false;
    switch (image_type) {
        case VK_IMAGE_TYPE_1D:
            compatible = (view_type == VK_IMAGE_VIEW_TYPE_1D && array_layers == 1) ||
                         (view_type == VK_IMAGE_VIEW_TYPE_1D_ARRAY);
            break;
        case VK_IMAGE_TYPE_2D:
            compatible = (view_type == VK_IMAGE_VIEW_TYPE_2D && array_layers == 1) ||
                         (view_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY) ||
                         (view_type == VK_IMAGE_VIEW_TYPE_CUBE && array_layers == 6 && (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)) ||
                         (view_type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY && (array_layers % 6 == 0) && (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT));
            break;
        case VK_IMAGE_TYPE_3D:
            compatible = (view_type == VK_IMAGE_VIEW_TYPE_3D && array_layers == 1) ||
                         ((view_type == VK_IMAGE_VIEW_TYPE_2D || view_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY) && (flags & VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT));
            break;
        default :
            compatible = false;
    }

    if (!compatible) {
        ev_log_error("[ev::ImageView] Image type and view type are not compatible.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return VK_SUCCESS;
}

void ImageView::destroy() {
    ev_log_info("[ev::ImageView::destroy] Destroying ImageView.");
    if (view != VK_NULL_HANDLE) {
        vkDestroyImageView(*device, view, nullptr);
        view = VK_NULL_HANDLE;
    }
    ev_log_info("[ev::ImageView::destroy] ImageView destroyed successfully.");
}

ImageView::~ImageView() {
    destroy();
}