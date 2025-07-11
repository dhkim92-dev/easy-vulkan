#pragma once

#include <memory>
#include <ev-device.h>
#include <ev-image.h>
#include <ev-image_view.h>
#include <ev-sampler.h>

namespace ev {

class Texture {
public:
    std::shared_ptr<ev::Image> image = nullptr;
    std::shared_ptr<ev::ImageView> image_view = nullptr;
    std::shared_ptr<ev::Sampler> sampler = nullptr;

    Texture(std::shared_ptr<ev::Image> image, 
            std::shared_ptr<ev::ImageView> image_view, 
            std::shared_ptr<ev::Sampler> sampler);

    ~Texture() = default;

    VkDescriptorImageInfo get_descriptor() const {
        VkDescriptorImageInfo descriptor = {};
        descriptor.imageView = *image_view;
        descriptor.sampler = *sampler;
        descriptor.imageLayout = image->get_layout();
        return descriptor;
    }
};

}