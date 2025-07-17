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

enum VertexType {
    Position,
    Normal,
    UV,
    Color,
    Joint,
    Weight,
    Tangent
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 color;
    glm::vec4 joint;
    glm::vec4 weight;
    glm::vec4 tangent;

    static VkVertexInputBindingDescription vertex_binding_description;
    static std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions;
    static VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info;
    static VkVertexInputBindingDescription input_binding_description(uint32_t binding);
    static VkVertexInputAttributeDescription input_attribute_description(
        uint32_t binding, 
        uint32_t location,
        VertexType type
    );
    static std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions(
        uint32_t binding, 
        std::vector<VertexType> types
    );
    static VkPipelineVertexInputStateCreateInfo* get_pipeline_vertex_input_state(const std::vector<VertexType> types);
};

class Skin {

private:

    std::string name;

    std::shared_ptr<Node> skeleton_root = nullptr;

    std::vector<glm::mat4> inverse_bind_matrices;

    std::vector<std::shared_ptr<Node>> joints;

public:

    Skin(const std::string& name) : name(name) {}

    void set_name(const std::string& name) {
        this->name = name;
    }

    void set_skeleton_root(std::shared_ptr<Node> root) {
        skeleton_root = std::move(root);
    }

    void add_inverse_bind_matrix(const glm::mat4& matrix) {
        inverse_bind_matrices.push_back(matrix);
    }

    void add_joint(std::shared_ptr<Node> joint) {
        joints.push_back(std::move(joint));
    }

    const std::string& get_name() const {
        return name;
    }

    const std::shared_ptr<Node>& get_skeleton_root() const {
        return skeleton_root;
    }

    const std::vector<glm::mat4>& get_inverse_bind_matrices() const {
        return inverse_bind_matrices;
    }

    const std::vector<std::shared_ptr<Node>>& get_joints() const {
        return joints;
    }
};

class Animation {

public:

    struct AnimationSampler {
        enum InterpolationMethod {
            LINEAR,
            STEP,
            CUBICSPLINE
        };
        
        InterpolationMethod method;

        std::vector<float> input_times;

        std::vector<glm::vec4> outputs;
    };

    struct AnimationChannel {

        enum PathType { TRANSLATION, ROTATION, SCALE };

        PathType path_type;

        std::shared_ptr<Node> target_node;

        uint32_t sampler_index;
    };

private:
 
    std::string name;

    std::vector<AnimationSampler> samplers;

    std::vector<AnimationChannel> channels;

    float start= std::numeric_limits<float>::max();

    float end = std::numeric_limits<float>::min();

public:
    
    Animation(const std::string& name) : name(name) {}

    void set_name(const std::string& name) {
        this->name = name;
    }

    void add_sampler(const AnimationSampler& sampler) {
        samplers.push_back(sampler);
    }

    void add_channel( const AnimationChannel& channel) {
        channels.emplace_back(channel);
    }

    void set_start_time(float time) {
        start = std::min(start, time);
    }

    void set_end_time(float time) {
        end = std::max(end, time);
    }

    const std::string& get_name() const {
        return name;
    }

    std::vector<AnimationSampler>& get_samplers() {
        return samplers;
    }

    const std::vector<AnimationChannel>& get_channels() const {
        return channels;
    }

    float get_start_time() const {
        return start;
    }

    float get_end_time() const {
        return end;
    }
};

class Material {

public: 

    enum AlphaMode {
        OPAQUE,
        MASK,
        BLEND
    };

private:

    std::shared_ptr<ev::Device> device = nullptr;

    AlphaMode alpha_mode = OPAQUE;

    float alpha_cutoff = 1.0f;

    float metallic_factor = 1.0f;

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

public:
    
    Material(std::shared_ptr<ev::Device> device)
        : device(std::move(device)) {
        if (!this->device) {
            ev::logger::Logger::getInstance().error("[ev::tools::gltf::Material] Device is null");
            exit(EXIT_FAILURE);
        }
    }

    void set_alpha_mode(AlphaMode mode) {
        alpha_mode = mode;
    }

    void set_alpha_cutoff(float cutoff) {
        alpha_cutoff = cutoff;
    }

    void set_metallic_factor(float factor) {
        metallic_factor = factor;
    }

    void set_roughness_factor(float factor) {
        roughness_factor = factor;
    }

    void set_double_sided(bool double_sided) {
        this->double_sided = double_sided;
    }

    void set_base_color_factor(const glm::vec4& color) {
        base_color_factor = color;
    }

    void set_base_color_texture(std::shared_ptr<ev::Texture> texture) {
        base_color_texture = std::move(texture);
    }

    void set_metallic_roughness_texture(std::shared_ptr<ev::Texture> texture) {
        metallic_roughness_texture = std::move(texture);
    }

    void set_normal_texture(std::shared_ptr<ev::Texture> texture) {
        normal_texture = std::move(texture);
    }

    void set_occlusion_texture(std::shared_ptr<ev::Texture> texture) {
        occlusion_texture = std::move(texture);
    }

    void set_emissive_texture(std::shared_ptr<ev::Texture> texture) {
        emissive_texture = std::move(texture);
    }

    void set_diffuse_texture(std::shared_ptr<ev::Texture> texture) {
        diffuse_texture = std::move(texture);
    }

    void set_specular_texture(std::shared_ptr<ev::Texture> texture) {
        specular_texture = std::move(texture);
    }

    void set_descriptor_set(std::shared_ptr<ev::DescriptorSet> descriptor_set) {
        this->descriptor_set = std::move(descriptor_set);
    }

    std::shared_ptr<ev::DescriptorSet> get_descriptor_set() const {
        return descriptor_set;
    }

    AlphaMode get_alpha_mode() const {
        return alpha_mode;
    }

    float get_alpha_cutoff() const {
        return alpha_cutoff;
    }

    float get_metallic_factor() const {
        return metallic_factor;
    }

    float get_roughness_factor() const {
        return roughness_factor;
    }

    bool is_double_sided() const {
        return double_sided;
    }

    const glm::vec4& get_base_color_factor() const {
        return base_color_factor;
    }

    std::shared_ptr<ev::Texture> get_base_color_texture() const {
        return base_color_texture;
    }

    std::shared_ptr<ev::Texture> get_metallic_roughness_texture() const {
        return metallic_roughness_texture;
    }

    std::shared_ptr<ev::Texture> get_normal_texture() const {
        return normal_texture;
    }

    std::shared_ptr<ev::Texture> get_occlusion_texture() const {
        return occlusion_texture;
    }

    std::shared_ptr<ev::Texture> get_emissive_texture() const {
        return emissive_texture;
    }

    std::shared_ptr<ev::Texture> get_diffuse_texture() const {
        return diffuse_texture;
    }

    std::shared_ptr<ev::Texture> get_specular_texture() const {
        return specular_texture;
    }
};

class Primitive {

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

public: 

    Primitive(uint32_t first_index, 
        uint32_t index_count, 
        uint32_t first_vertex, 
        uint32_t vertex_count, 
        std::shared_ptr<Material> material
    ): first_index(first_index), index_count(index_count), first_vertex(first_vertex), vertex_count(vertex_count), material(std::move(material)) {}

    void set_dimensions(const glm::vec3 min, const glm::vec3 max);

    const std::shared_ptr<Material> get_material() const {
        return material;
    }

    const uint32_t get_first_index() const {
        return first_index;
    }

    const uint32_t get_index_count() const {
        return index_count;
    }

    const uint32_t get_first_vertex() const {
        return first_vertex;
    }

    const uint32_t get_vertex_count() const {
        return vertex_count;
    }
};

class Mesh {

public:
    struct UniformBuffer {
        std::shared_ptr<ev::Buffer> buffer;
        std::shared_ptr<ev::DescriptorSet> descriptor_set;
    };

    struct Uniform {
        glm::mat4 matrix;
        glm::mat4 joint_matrix[64]{};
        float joint_count = 0.0f;
    };

private:

    std::vector<std::shared_ptr<Primitive>> primitives;

    std::string name;

    std::shared_ptr<ev::Buffer> buffer = nullptr;

    std::shared_ptr<ev::DescriptorSet> descriptor_set = nullptr;

    Uniform uniform_data;

    UniformBuffer uniform_buffer;

public:

    Mesh() = default;

    void add_primitive(std::shared_ptr<Primitive> primitive) {
        primitives.push_back(std::move(primitive));
    }

    void set_uniform_buffer(std::shared_ptr<ev::Buffer> buffer) {
        uniform_buffer.buffer = std::move(buffer);
    }

    void set_descriptor_set(std::shared_ptr<ev::DescriptorSet> descriptor_set) {
        uniform_buffer.descriptor_set = std::move(descriptor_set);
    }

    void set_name(const std::string& name) {
        this->name = name;
    }

    const std::vector<std::shared_ptr<Primitive>>& get_primitives() const {
        return primitives;
    }

    const std::string& get_name() const {
        return name;
    }

    std::shared_ptr<ev::Buffer>& get_uniform_buffer() {
        return uniform_buffer.buffer;
    }

    std::shared_ptr<ev::DescriptorSet>& get_descriptor_set() {
        return uniform_buffer.descriptor_set;
    }

    Uniform& get_uniform_data() {
        return uniform_data;
    }
};

class Node : public std::enable_shared_from_this<Node> {

private:

    /** 
     * 자기 참조로 인한 메모리 누수방지를 위해 parent 는 weak_ptr 선언  
     * @note 모든 노드는 모델을 통해 루트가 소유되므로 예상치 못한 메모리 해제는 발생하지 않음
    */
    std::weak_ptr<Node> parent; 

    std::string name;

    std::vector<std::shared_ptr<Node>> children;

    std::shared_ptr<Skin> skin;

    std::shared_ptr<Mesh> mesh;

    uint32_t skin_index = -1;

    uint32_t index;

    glm::mat4 matrix;

    glm::vec3 translation{};

    glm::quat rotation{};

    glm::vec3 scale{1.0f};

public:

    explicit Node(uint32_t index);

    void set_parent(const std::shared_ptr<Node>& parent_node) {
        parent = parent_node;
    }

    void add_child(const std::shared_ptr<Node>& child_node) {
        children.emplace_back(child_node);
        child_node->set_parent(shared_from_this());
    }

    void clear_children() {
        children.clear();
    }

    void set_name(const std::string& name) {
        this->name = name;
    }

    void set_translation(const glm::vec3& translation) {
        this->translation = translation;
        update_matrix();
    }

    void set_rotation(const glm::quat& rotation) {
        this->rotation = rotation;
        update_matrix();
    }

    void set_scale(const glm::vec3& scale) {
        this->scale = scale;
        update_matrix();
    }

    void set_index(const uint32_t index) {
        this->index = index;
    }

    void set_skin_index(const uint32_t skin_index) {
        this->skin_index = skin_index;
    }

    void set_matrix(const glm::mat4& matrix) {
        this->matrix = matrix;
    }

    void update_matrix() {
        matrix = glm::mat4(1.0f);
        matrix = glm::translate(matrix, translation);
        matrix *= glm::mat4_cast(rotation);
        matrix = glm::scale(matrix, scale);
    }

    std::weak_ptr<Node>& get_parent() {
        return parent;
    }

    const glm::mat4& get_matrix() const {
        return matrix;
    }

    const std::string& get_name() const {
        return name;
    }

    const uint32_t get_index() const {
        return index;
    }

    const std::vector<std::shared_ptr<Node>>& get_children() const {
        return children;
    }

    const std::shared_ptr<Skin>& get_skin() const {
        return skin;
    }

    void set_skin(const std::shared_ptr<Skin>& skin) {
        this->skin = skin;
    }

    const std::shared_ptr<Mesh>& get_mesh() const {
        return mesh;
    }

    const uint32_t get_skin_index() const {
        return skin_index;
    }

    void set_mesh(const std::shared_ptr<Mesh>& mesh) {
        this->mesh = mesh;
    }
};


enum RenderFlag {
    BIND_IMAGE = 0x01,
    OPAQUE = 0x02,
    ALPHA_MASK = 0x04,
    ALPHA_BLEND = 0x08
};

class Model {
    
private : 

    std::shared_ptr<ev::Device> device = nullptr;

    std::vector<std::shared_ptr<Node>> nodes;

    std::vector<std::shared_ptr<Node>> linear_nodes;

    std::vector<std::shared_ptr<Skin>> skins;

    std::vector<std::shared_ptr<Material>> materials;

    std::vector<std::shared_ptr<Animation>> animations;

    std::vector<std::shared_ptr<ev::Texture>> textures;

    std::shared_ptr<ev::Buffer> vertex_buffer = nullptr;

    std::shared_ptr<ev::Buffer> index_buffer = nullptr;

    std::vector<std::shared_ptr<ev::DescriptorSetLayout>> descriptor_set_layouts;

    bool buffer_bound = false;

    void bind_node_descriptor_sets(
        std::shared_ptr<ev::CommandBuffer> command_buffer, 
        const std::shared_ptr<Node>& node
    );

    void draw_node(
        std::shared_ptr<ev::CommandBuffer> command_buffer,
        const std::shared_ptr<Node> node,
        uint32_t render_flags = RenderFlag::OPAQUE | RenderFlag::BIND_IMAGE,
        std::shared_ptr<ev::PipelineLayout> pipeline_layout = nullptr,
        uint32_t bind_image_set = 1
    );

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

    const std::vector<std::shared_ptr<Node>>& get_nodes() const {
        return nodes;
    }

    const std::shared_ptr<Node> get_node_by_index(uint32_t index) const;

    const std::vector<std::shared_ptr<Skin>>& get_skins() const {
        return skins;
    }

    const std::vector<std::shared_ptr<Material>>& get_materials() const {
        return materials;
    }

    const std::vector<std::shared_ptr<Animation>>& get_animations() const {
        return animations;
    }

    const std::vector<std::shared_ptr<ev::Texture>>& get_textures() const {
        return textures;
    }

    const std::vector<std::shared_ptr<Node>>& get_linear_nodes() const {
        return linear_nodes;
    }

    void add_node(const std::shared_ptr<Node>& node) {
        nodes.emplace_back(node);
    }

    void add_skin(const std::shared_ptr<Skin>& skin) {
        skins.emplace_back(skin);
    }

    void add_material(const std::shared_ptr<Material>& material) {
        materials.emplace_back(material);
    }

    void add_animation(const std::shared_ptr<Animation>& animation) {
        animations.emplace_back(animation);
    }

    void add_texture(const std::shared_ptr<ev::Texture>& texture) {
        textures.emplace_back(texture);
    }

    void add_linear_node(const std::shared_ptr<Node>& node) {
        linear_nodes.emplace_back(node);
    }

    void add_descriptor_set_layout(std::shared_ptr<ev::DescriptorSetLayout> layout);

    void bind_buffers(std::shared_ptr<ev::CommandBuffer> command_buffer);

    void draw(std::shared_ptr<ev::CommandBuffer> command_buffer, 
        uint32_t render_flags = RenderFlag::OPAQUE | RenderFlag::BIND_IMAGE,
        std::shared_ptr<ev::PipelineLayout> pipeline_layout = nullptr,
        uint32_t bind_image_set = 1
    );

    const std::shared_ptr<ev::Buffer> get_vertex_buffer() const {
        return vertex_buffer;
    }

    const std::shared_ptr<ev::Buffer> get_index_buffer() const {
        return index_buffer;
    }

    void set_vertex_buffer(std::shared_ptr<ev::Buffer> buffer) {
        vertex_buffer = std::move(buffer);
    }

    void set_index_buffer(std::shared_ptr<ev::Buffer> buffer) {
        index_buffer = std::move(buffer);
    }
};


enum DescriptorBindingFlags {
    ImageBaseColor = 0x01,
    ImageNormalMap = 0x02
};

/**
 * @brief GLTFModelManager GLTF 모델을 반환하는 객체입니다.
 */
class GLTFModelManager {

private: 

    std::shared_ptr<ev::Device> device = nullptr;

    std::shared_ptr<ev::DescriptorPool> descriptor_pool = nullptr;

    std::shared_ptr<ev::MemoryAllocator> memory_allocator = nullptr;

    std::shared_ptr<ev::CommandPool> command_pool = nullptr;

    std::shared_ptr<ev::Queue> transfer_queue = nullptr;

    std::filesystem::path resource_path;

    DescriptorBindingFlags descriptor_binding_flags = DescriptorBindingFlags::ImageBaseColor;

    std::shared_ptr<ev::Texture> load_texture(tinygltf::Image& image, std::string& file_path);

    void load_textures(tinygltf::Model& gltf_model, std::shared_ptr<Model> model);

    void load_materials(tinygltf::Model& gltf_model, std::shared_ptr<Model> model);

    void load_node_meshes(
        tinygltf::Model& gltf_model,
        std::shared_ptr<ev::tools::gltf::Model> model,
        tinygltf::Node& node,
        std::shared_ptr<Node> new_node,
        std::vector<Vertex> &h_vertices,
        std::vector<uint32_t> &h_indices
    );

    void load_nodes(tinygltf::Model& gltf_model, 
        std::shared_ptr<Model> model,
        std::vector<Vertex> &h_vertices,
        std::vector<uint32_t> &h_indices,
        float scale_factor = 1.0f
    );

    /**
     * @brief 단일 glTF 노드를 로드합니다.
     * @param gltf_model tiny gltf 모델 객체
     * @param model ev::tools::gltf::Model 객체
     */
    void load_node(
        std::shared_ptr<ev::tools::gltf::Node> parent_node,
        tinygltf::Model& gltf_model,
        std::shared_ptr<Model> model,
        tinygltf::Node& node,
        uint32_t node_idx,
        std::vector<Vertex> &h_vertices,
        std::vector<uint32_t> &h_indices,
        float scale_factor = 1.0f
    );

    void add_mesh_vertices(
        tinygltf::Model& gltf_model,
        tinygltf::Mesh& mesh,
        const tinygltf::Primitive& primitive,
        std::vector<Vertex> &h_vertices,
        glm::vec3 &min_pos,
        glm::vec3 &max_pos,
        uint32_t& vertex_start,
        uint32_t& vertex_count
    );

    void add_mesh_indices(
        tinygltf::Model& gltf_model,
        std::shared_ptr<ev::tools::gltf::Model> model,
        tinygltf::Mesh& mesh,
        std::shared_ptr<ev::tools::gltf::Mesh> new_mesh,
        const tinygltf::Primitive& primitive,
        std::vector<uint32_t> &h_indices,
        uint32_t& vertex_start,
        uint32_t& vertex_count,
        uint32_t& index_start,
        uint32_t& index_count,
        glm::vec3 &min_pos,
        glm::vec3 &max_pos
    );

    void load_skins(tinygltf::Model& gltf_model, std::shared_ptr<Model> model);

    void load_animations(tinygltf::Model& gltf_model, std::shared_ptr<Model> model);

    std::shared_ptr<ev::Texture> get_texture(std::shared_ptr<Model> model, uint32_t idx);

    void setup_vertex_buffer(
        std::shared_ptr<ev::tools::gltf::Model> model, 
        std::vector<Vertex> &h_vertices
    );

    void setup_index_buffer(
        std::shared_ptr<ev::tools::gltf::Model> model, 
        std::vector<uint32_t> &h_indices
    );

    void prepare_material_descriptor_sets(
        std::shared_ptr<ev::tools::gltf::Model> model
    );

    void prepare_node_descriptor_set(
        std::shared_ptr<ev::tools::gltf::Model> model, 
        std::shared_ptr<Node> node, 
        std::shared_ptr<DescriptorSetLayout> layout
    );

    void prepare_node_descriptor_sets(
        std::shared_ptr<ev::tools::gltf::Model> model
    );

public: 
  
    /**
     * @brief GLTFModelManager 생성자입니다.
     * @param device Vulkan 디바이스 객체
     * @param memory_allocator 메모리 할당기 객체
     * @param descriptor_pool 디스크립터 풀 객체
     * @param command_pool 커맨드 풀 객체
     * @param transfer_queue 전송 큐 객체
     */
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

    void set_descriptor_binding_flags(DescriptorBindingFlags flags) {
        descriptor_binding_flags = flags;
    }

    // void save_model(std::shared_ptr<Model> model, const std::string save_path);
};

}