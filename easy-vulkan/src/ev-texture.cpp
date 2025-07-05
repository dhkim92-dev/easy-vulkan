#include "ev-texture.h"

using namespace ev;

Texture::Texture(
    std::shared_ptr<ev::Image> image, 
    std::shared_ptr<ev::ImageView> image_view, 
    std::shared_ptr<ev::Sampler> sampler
) : image(std::move(image)), image_view(std::move(image_view)), sampler(std::move(sampler)) {
    if (!this->image || !this->image_view || !this->sampler) {
        throw std::runtime_error("Failed to create Texture: Image, ImageView or Sampler is null.");
    }
}
