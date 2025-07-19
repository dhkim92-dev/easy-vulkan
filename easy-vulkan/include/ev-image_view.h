#pragma once

#include "ev-device.h"
#include "ev-image.h"
#include "ev-logger.h"

namespace ev {

/**
 * @brief 이미지 뷰 생성자
 * @param device Vulkan 디바이스
 * @param image 이미지 객체의 shared_ptr
 * @param view_type 이미지 뷰 타입 (기본값: VK_IMAGE_VIEW_TYPE_MAX_ENUM)
 * @param format 이미지 뷰 포맷 (기본값: VK_FORMAT_UNDEFINED)
 * @param components 컴포넌트 매핑 (기본값: {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY})
 * @param subresource_range 서브리소스 범위 (기본값: {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
 */
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

    /**
     * @brief 이미지 뷰 생성자
     * @param device Vulkan 디바이스
     * @param image 이미지 객체의 shared_ptr
     * @param view_type 이미지 뷰 타입 (기본값: VK_IMAGE_VIEW_TYPE_MAX_ENUM)
     * @param format 이미지 뷰 포맷 (기본값: VK_FORMAT_UNDEFINED)
     * @param components 컴포넌트 매핑 (기본값: {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY})
     * @param subresource_range 서브리소스 범위 (기본값: {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
     */
    explicit ImageView(shared_ptr<Device> device, 
        shared_ptr<Image> image,
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM,
        VkFormat format = VK_FORMAT_UNDEFINED,
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

    std::shared_ptr<Image> get_image() const {
        return image;
    }

    operator VkImageView() const {
        return view;
    }
};

}
