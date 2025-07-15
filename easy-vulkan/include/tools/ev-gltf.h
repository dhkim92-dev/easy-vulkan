#pragma once
#include <vector>
#include <memory>
#include <filesystem>
#include "ev-logger.h"
#include "ev-device.h"
#include "ev-texture.h"
#include "ev-memory.h"
#include "ev-memory_allocator.h"
#include "ev-queue.h"
#include "ev-command_pool.h"
#include "ev-buffer.h"
#include "ev-image.h"
#include "ev-texture.h"
#include "ev-image_view.h"
#include "ev-sampler.h"
#include "ev-descriptor_set.h"
// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// #define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace ev;

namespace ev::tools::gltf {

class Node;
class Material;
class Vertices;
class Indices;
class Mesh;
class Skin;
class Animation;
class Vertex;

class Material {

private:

    std::shared_ptr<ev::Device> device = nullptr;

public:

    enum AlphaMode {
        OPAQUE,
        MASK,
        BLEND
    };

    AlphaMode alpha_mode = OPAQUE;

    float alpha_cutoff = 1.0f;

    float metalic_factor = 1.0f;

    float roughness_factor = 1.0f;

    bool double_sided = false;

    glm::vec4 base_color_factor = glm::vec4(1.0f);

    std::shared_ptr<ev::Texture> base_color_texture = nullptr;

    std::shared_ptr<ev::Texture> metallic_roughness_texture = nullptr;

    std::shared_ptr<ev::Texture> normal_texture = nullptr;

    std::shared_ptr<ev::Texture> occlusion_texture = nullptr;

    std::shared_ptr<ev::Texture> emissive_texture = nullptr;

    std::shared_ptr<ev::Texture> diffuse_texture = nullptr;

    std::shared_ptr<ev::Texture> specular_texture = nullptr;

    std::shared_ptr<ev::DescriptorSet> descriptor_set = nullptr;
};

struct Primitive {

    uint32_t first_index;

    uint32_t index_count;

    uint32_t first_vertex;

    uint32_t vertex_count;

    std::shared_ptr<Material> material;

    struct Dimensions {

        glm::vec3 min = glm::vec3(FLT_MAX);

        glm::vec3 max = glm::vec3(-FLT_MAX);

        glm::vec3 size;

        glm::vec3 center;

        float radius = 0.0f;
    } dimensions;

    void set_dimensions(const glm::vec3 min, const glm::vec3 max);

    Primitive(uint32_t first_index, uint32_t index_count, uint32_t first_vertex, uint32_t vertex_count, std::shared_ptr<Material> material)
        : first_index(first_index), index_count(index_count), first_vertex(first_vertex), vertex_count(vertex_count), material(std::move(material)) {}
};

class Mesh {

    private:

    std::shared_ptr<ev::Device> device = nullptr;

    std::vector<Primitive> primitives;

    std::string name;

    std::shared_ptr<ev::Buffer> buffer = nullptr;

    std::shared_ptr<ev::DescriptorSet> descriptor_set = nullptr;
};

class Model {
    
private : 

    std::shared_ptr<ev::Device> device = nullptr;

    std::vector<std::shared_ptr<Node>> nodes;

    std::vector<std::shared_ptr<Skin>> skins;

    std::vector<std::shared_ptr<Material>> materials;

    std::vector<std::shared_ptr<Animation>> animations;

    std::vector<std::shared_ptr<ev::Texture>> textures;

public:

    explicit Model(std::shared_ptr<ev::Device> device)
        : device(std::move(device)) {
        if (!this->device) {
            ev::logger::Logger::getInstance().error("[ev::tools::gltf::Model] Device is null");
            exit(EXIT_FAILURE);
        }
    }

    void set_nodes(const std::vector<std::shared_ptr<Node>>& nodes) {
        this->nodes = nodes;
    }

    void set_skins(const std::vector<std::shared_ptr<Skin>>& skins) {
        this->skins = skins;
    }

    void set_materials(const std::vector<std::shared_ptr<Material>>& materials) {
        this->materials = materials;
    }

    void set_animations(const std::vector<std::shared_ptr<Animation>>& animations) {
        this->animations = animations;
    }

    void set_textures(const std::vector<std::shared_ptr<ev::Texture>>& textures) {
        this->textures = textures;
    }

    std::vector<std::shared_ptr<Node>>& get_nodes() {
        return nodes;
    }

    std::vector<std::shared_ptr<Skin>>& get_skins() {
        return skins;
    }

    std::vector<std::shared_ptr<Material>>& get_materials() {
        return materials;
    }

    std::vector<std::shared_ptr<Animation>>& get_animations() {
        return animations;
    }

    std::vector<std::shared_ptr<ev::Texture>>& get_textures() {
        return textures;
    }
};

class GLTFModelManager {

private: 

    std::shared_ptr<ev::Device> device = nullptr;

    std::shared_ptr<ev::DescriptorPool> descriptor_pool = nullptr;

    std::shared_ptr<ev::MemoryAllocator> memory_allocator = nullptr;

    std::shared_ptr<ev::CommandPool> command_pool = nullptr;

    std::shared_ptr<ev::Queue> transfer_queue = nullptr;

    std::filesystem::path resource_path;

    std::shared_ptr<ev::Texture> load_texture(tinygltf::Image& image, std::string& file_path);

    void load_textures(tinygltf::Model* gltf_model, std::shared_ptr<Model> model);

    void load_materials(tinygltf::Model* gltf_model, std::shared_ptr<Model> model);

    void load_nodes(tinygltf::Model* gltf_model, std::shared_ptr<Model> model);

    void load_skins(tinygltf::Model* gltf_model, std::shared_ptr<Model> model);

    void load_animations(tinygltf::Model* gltf_model, std::shared_ptr<Model> model);

public: 
    
    explicit GLTFModelManager(
        std::shared_ptr<ev::Device> device,
        std::shared_ptr<ev::MemoryAllocator> memory_allocator,
        std::shared_ptr<ev::DescriptorPool> descriptor_pool,
        std::shared_ptr<ev::CommandPool> command_pool,
        std::shared_ptr<ev::Queue> transfer_queue
        // std::filesystem::path resource_path = std::filesystem::current_path() / "resources"
    );

    ~GLTFModelManager() = default;

    std::shared_ptr<Model> load_model(
        const std::string file_path
    );

    // void save_model(std::shared_ptr<Model> model, const std::string save_path);
};

}