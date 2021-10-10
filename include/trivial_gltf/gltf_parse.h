/* ==========================================================================
 Copyright (c) 2021 Andreas Pokorny
 Distributed under the Boost Software License, Version 1.0. (See accompanying
 file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
========================================================================== */

#ifndef TRIVIAL_GLTF_GLTF_PARSE_H_INCLUDED
#define TRIVIAL_GLTF_GLTF_PARSE_H_INCLUDED

#include <tiny_tuple/tuple.h>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <variant>
#include <utility>
#include <functional>
#include <span>

// TODO most of the names are not necessary for anything - consider dropping / skipping
namespace trivial_gltf
{
struct node
{
    int32_t               mesh{-1};
    int32_t               skin{-1};
    int32_t               camera{-1};
    glm::qua<float>       rotaton{};
    glm::vec<3, float>    scale{};
    glm::vec<3, float>    translation{};
    std::vector<uint32_t> children;
    std::vector<uint32_t> weights;
    std::string           name;
    // not done extensions and extras
};

struct skin
{
    std::string           name;
    int32_t               skeleton;
    int32_t               inverseBindMaterials;
    std::vector<uint32_t> joints;
};

struct scene
{
    std::string           name;
    std::vector<uint32_t> root_nodes;
};

enum class component : uint16_t
{
    byte_type           = 5120,
    unsigned_byte_type  = 5121,
    short_type          = 5122,
    unsigned_short_type = 5123,
    unsigned_int_type   = 5125,
    float_type          = 5126,
};

enum class attribute_type : uint8_t
{
    scalar,
    vec2,
    vec3,
    vec4,
    mat2,
    mat3,
    mat4
};

using attribute_scalar_type = std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, float>;
using attribute_value_type =
    std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, float, glm::vec<2, int8_t>, glm::vec<2, uint8_t>,
                 glm::vec<2, int16_t>, glm::vec<2, uint16_t>, glm::vec<2, int32_t>, glm::vec<2, uint32_t>, glm::vec<2, float>,
                 glm::vec<3, int8_t>, glm::vec<3, uint8_t>, glm::vec<3, int16_t>, glm::vec<3, uint16_t>, glm::vec<3, int32_t>,
                 glm::vec<3, uint32_t>, glm::vec<3, float>, glm::vec<4, int8_t>, glm::vec<4, uint8_t>, glm::vec<4, int16_t>,
                 glm::vec<4, uint16_t>, glm::vec<4, int32_t>, glm::vec<4, uint32_t>, glm::vec<4, float>, glm::tmat2x2<int8_t>,
                 glm::tmat2x2<uint8_t>, glm::tmat2x2<int16_t>, glm::tmat2x2<uint16_t>, glm::tmat2x2<int32_t>, glm::tmat2x2<uint32_t>,
                 glm::tmat2x2<float>, glm::tmat3x3<int8_t>, glm::tmat3x3<uint8_t>, glm::tmat3x3<int16_t>, glm::tmat3x3<uint16_t>,
                 glm::tmat3x3<int32_t>, glm::tmat3x3<uint32_t>, glm::tmat3x3<float>, glm::tmat4x4<int8_t>, glm::tmat4x4<uint8_t>,
                 glm::tmat4x4<int16_t>, glm::tmat4x4<uint16_t>, glm::tmat4x4<int32_t>, glm::tmat4x4<uint32_t>, glm::tmat4x4<float>>;

enum class attribute : uint8_t
{
    position,
    normal,
    tangent,
    texcoord_0,
    texoord_1,
    color_0,
    joints_0,
    weights_0,
    extended_attribute  // further attributes for shaders go beyond this
};

template <attribute v>
constexpr int value = static_cast<int>(v);

enum class attribute_flag : uint32_t
{
    position_flag   = 1 << value<attribute::position>,
    normal_flag     = 1 << value<attribute::normal>,
    tangent_flag    = 1 << value<attribute::tangent>,
    texcoord_0_flag = 1 << value<attribute::texcoord_0>,
    texoord_1_flag  = 1 << value<attribute::texoord_1>,
    color_0_flag    = 1 << value<attribute::color_0>,
    joints_0_flag   = 1 << value<attribute::joints_0>,
    weights_0_flag  = 1 << value<attribute::weights_0>,
};

enum class mode_type : uint8_t
{
    points,
    lines,
    line_loop,
    line_strip,
    triangles,
    triangles_strip,
    triangle_fan
};

enum class interpolation_type : uint8_t
{
    linear,
    step,
    cubic_spline
};

enum class path_type : uint8_t
{
    scale,
    rotattion,
    translation,
    weights
};

using attribute_offset = tiny_tuple::tuple<attribute, uint32_t>;

struct primitive
{
    std::vector<attribute_offset> attributes;
    int32_t                       indices{-1};   // accessor
    int32_t                       material{-1};  // index of material
    mode_type                     mode{mode_type::triangles};
    attribute_flag                flags;
    // dont understand morph targets yet
};
struct mesh
{
    std::string            name;
    std::vector<primitive> primitives;
    std::vector<float>     weights;
};

struct channel
{
    int32_t   sampler_id;
    int32_t   node_id;
    path_type path;  // scale rot translate weights
};

struct animation_sampler
{
    int32_t            input;
    int32_t            output;
    interpolation_type interpolation;  // linear step cubicspline
};

struct animation
{
    std::string                    name;
    std::vector<channel>           channels;
    std::vector<animation_sampler> samplers;
};

struct texture_info
{
    int32_t     index;
    int32_t     tex_coord{0};
    std::string name;
};

struct normal_texture_info : texture_info
{
    float scale;
};
struct occlusion_texture_info : texture_info
{
    float strength;
};

struct pbr_metallic_roughness
{
    glm::vec<4, float> base_color_factor;
    texture_info       base_color_texture;
    float              metallic_factor;
    float              roughness_factor;
    texture_info       metallic_roughness_texture;
};

enum class alpha_mode_type
{
    opaque,
    mask,
    blend
};

struct material
{
    std::string            name;
    pbr_metallic_roughness data;
    normal_texture_info    normal;
    occlusion_texture_info occlusion;
    texture_info           emissive;
    glm::vec<3, float>     emssive_factor;
    alpha_mode_type        alpha_mode;
    float                  alpha_cut_off;
    bool                   double_sided;
};

struct accessor
{
    uint32_t           view;
    uint32_t           offset;
    uint32_t           count;
    component          comp_type;
    attribute_type     type;
    bool               normalized;
    std::vector<float> max;
    std::vector<float> min;

    // not supported sparse, name, extensions, extras
};

struct buffer_view
{
    uint32_t buffer;
    uint32_t length;
    uint32_t offset;
    uint32_t stride;
    uint32_t target;
};

struct infile_buffer
{
    size_t byte_length;
};
struct external_buffer
{
    size_t      byte_length;
    std::string uri;
};
using buffer = std::variant<infile_buffer, external_buffer>;

struct infile_image
{
    std::string name;
    std::string mime;
    int32_t     buffer_view;
};
struct external_image
{
    std::string name;
    std::string uri;
};
using image = std::variant<infile_image, external_image>;

struct sampler
{
    int32_t min_filter;
    int32_t mag_filter;
    int32_t wrap_s;
    int32_t wrap_t;
};
struct texture
{
    int32_t     sampler;
    int32_t     source;
    std::string name;
};

struct doc
{
    doc()                            = default;
    doc(doc const&)                  = delete;
    doc(doc&&)                       = delete;
    doc&                     operator=(doc const&) = delete;
    doc&                     operator=(doc&&) = delete;
    std::vector<scene>       scenes;
    std::vector<node>        nodes;
    std::vector<mesh>        meshes;
    std::vector<animation>   animations;
    std::vector<material>    materials;
    std::vector<accessor>    accessors;
    std::vector<buffer_view> buffer_views;
    std::vector<buffer>      buffers;
    std::vector<skin>        skins;
    std::vector<image>       images;
    std::vector<sampler>     samplers;
    std::vector<texture>     textures;
    /// const std::shared_ptr<cleanup_target> cleaner;
};

// who keeps the memory alive?
// TODO rethink the interface - there are too many cases...
// streaming over the network.. secondary resources?
// when to pull?
// from local meda - when to pull images / textures -> not at this step.. where to find
// transfrom gltf to something different?
enum class parse_state
{
    more_input_needed,
    error,
    json_complete
};

std::function<parse_state(std::span<char const> const&)> create_parser(doc& destination);
}  // namespace trivial_gltf
#endif
