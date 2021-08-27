/* ==========================================================================
 Copyright (c) 2021 Andreas Pokorny
 Distributed under the Boost Software License, Version 1.0. (See accompanying
 file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
========================================================================== */

#ifndef TRIVIAL_GLTF_GLTF_PARSE_H_INCLUDED
#define TRIVIAL_GLTF_GLTF_PARSE_H_INCLUDED

#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <variant>
#include <tiny_tuple/tuple.h>

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
                 glm::vec<4, uint16_t>, glm::vec<4, int32_t>, glm::vec<4, uint32_t>, glm::vec<4, float>, glm::mat<2, int8_t>,
                 glm::mat<2, uint8_t>, glm::mat<2, int16_t>, glm::mat<2, uint16_t>, glm::mat<2, int32_t>, glm::mat<2, uint32_t>,
                 glm::mat<2, float>, glm::mat<3, int8_t>, glm::mat<3, uint8_t>, glm::mat<3, int16_t>, glm::mat<3, uint16_t>,
                 glm::mat<3, int32_t>, glm::mat<3, uint32_t>, glm::mat<3, float>, glm::mat<4, int8_t>, glm::mat<4, uint8_t>,
                 glm::mat<4, int16_t>, glm::mat<4, uint16_t>, glm::mat<4, int32_t>, glm::mat<4, uint32_t>, glm::mat<4, float>>;

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

struct primitive
{
    attribute_flag                                      flags;
    int32_t                                             material{-1};  // index of material
    int32_t                                             indices{-1};   // accessor
    mode_type                                           mode{mode_type::triangles};
    std::vector<tiny_tuple::tuple<attribute, uint32_t>> attributes;
    // dont understand morph targets yet
};
struct mesh
{
    std::vector<primitive> primitives;
};

struct accessor
{
    uint32_t                                                             view;
    uint32_t                                                             offset;
    uint32_t                                                             count;
    component_type                                                       comp_type;
    attribute_type                                                       type;
    bool                                                                 normalized;
    std::vector<std::pair<attribute_scalar_type, attribute_scalar_type>> min_max;

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
using buffer = std::variant<infile_buffer, external_buffer, loaded_buffer>;

struct doc
{
    doc(doc const&)                               = delete;
    doc(doc&&)                                    = delete;
    doc&                                  operator=(doc const&) = delete;
    doc&                                  operator=(doc&&&) = delete;
    std::vector<node>                     nodes;
    std::vector<mesh>                     meshes;
    std::vector<buffer>                   buffers;
    const std::shared_ptr<cleanup_target> cleaner;
};
}  // namespace trivial_gltf
#endif
