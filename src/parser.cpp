/* ==========================================================================
 Copyright (c) 2021 Andreas Pokorny
 Distributed under the Boost Software License, Version 1.0. (See accompanying
 file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
========================================================================== */

#include "parser.h"

#include <async_json/on_array_element.hpp>
#include <async_json/is_path.hpp>
#include <algorithm>
#include <numeric>

namespace trivial_gltf
{
struct keyword
{
    char const* word;
    uint32_t    offset{0};
};
keyword interpolation_keywords[] = {{"LINEAR"}, {"STEP"}, {"CUBICSPLNE"}};
keyword type_keywords[]          = {{"SCALAR"}, {"VEC2"}, {"VEC3"}, {"VEC4"}, {"MAT2"}, {"MAT3"}, {"MAT4"}};
keyword path_keywords[]          = {{"scale"}, {"rotation"}, {"translation"}, {"weights"}};
keyword attribute_names[]        = {{"POSITION"},   {"NORMAL"},  {"TANGENT"},  {"TEXCOORD_0"},
                             {"TEXCOORD_1"}, {"COLOR_0"}, {"JOINTS_0"}, {"WEIGHTS_0"}};
keyword alpha_mode_keywords[]    = {{"OPAQUE"}, {"MASK"}, {"BLEND"}};

bool test(std::string_view const& l, char const* r)
{
    auto lb = l.begin();
    auto le = l.end();
    while (*r && lb != le)
        if (*lb++ != *r++) return false;
    return lb == le;
}

template <size_t N>
constexpr auto resolve_keywords(keyword (&kws)[N], int32_t& param)
{
    return [&param, kws](auto const& ev) mutable
    {
        if (ev.event == async_json::saj_event::string_value_start || ev.event == async_json::saj_event::string_value_cont)
        {
            for (auto& item : kws)
                if (test(ev.as_string_view(), item.word + item.offset))
                    item.offset += ev.as_string_view().size();
                else
                    item.offset = 0;
        }
        else if (ev.event == async_json::saj_event::string_value_end)
        {
            for (auto& item : kws)
                if (item.word[item.offset] == '\0')
                {
                    param       = &item - &(kws[0]);
                    item.offset = 0;
                }
        }
    };
}

constexpr auto resolve_path(int32_t& param) { return resolve_keywords(path_keywords, param); }
constexpr auto resolve_attribute(int32_t& param) { return resolve_keywords(attribute_names, param); }
constexpr auto resolve_animation_style(int32_t& param) { return resolve_keywords(interpolation_keywords, param); }
constexpr auto resolve_type(int32_t& param) { return resolve_keywords(type_keywords, param); }
constexpr auto resolve_alpha_mode(int32_t& param) { return resolve_keywords(alpha_mode_keywords, param); }

}  // namespace trivial_gltf

std::function<trivial_gltf::parse_state(std::span<char const> const&)> trivial_gltf::create_parser(trivial_gltf::doc& dest)
{
    namespace a = async_json;
    struct internal_state
    {
        int32_t                        id1{-1}, id2{-1}, id4{-1}, id5{-1};
        int32_t                        id3_nd{0};
        int32_t                        wrap_s{10497}, wrap_t{10497};
        int32_t                        draw_mode{4};
        float                          alpha_cut_off{0.5f};
        float                          fac1{1.0f};
        float                          fac2{1.0f};
        texture_info                   base_color_texture;
        texture_info                   metallic_roughness_texture;
        texture_info                   emissive_texture;
        normal_texture_info            normal_texture;
        occlusion_texture_info         occlusion_texture;
        bool                           flag1{false};
        std::vector<uint32_t>          u_numbers;
        std::vector<uint32_t>          node_numbers;
        std::vector<float>             f_numbers1;
        std::vector<float>             f_numbers2;
        std::vector<channel>           channels;
        std::vector<animation_sampler> samplers;
        std::vector<attribute_offset>  attribute_data;
        std::vector<primitive>         primitives;
        std::string                    name_str, uri;
        glm::vec3                      scale{1.0f, 1.0f, 1.0f};
        glm::vec3                      translation{.0f, .0f, .0f};
        glm::vec4                      color{1.0f, 1.0f, 1.0f, 1.0f};
        glm::qua<float>                rotation;

        void reset_parse_state() noexcept { *this = internal_state{}; }
        void reset_ids() noexcept
        {
            id1 = id2 = id4 = id5 = -1;
            id3_nd                = 0;
            draw_mode             = 4;
            wrap_s = wrap_t = 10497;
        }
    } p;
    auto parse_texture = [](char const* tex_attrib, texture_info& info)
    {
        return a::path(a::all(                                                      //
                           a::path(a::assign_numeric(info.index), "index"),         //
                           a::path(a::assign_numeric(info.tex_coord), "texCoord"),  //
                           a::path(a::assign_string(info.name), "name")             //
                           ),                                                       //
                       tex_attrib);
    };
    return [&dest, p,
            extractor = a::make_extractor(  //
                [parse_texture](a::error_cause er) { /* todo add error handling for illegal */ },
                a::path(                                                      //
                    a::all(                                                   //
                        a::path(a::assign_string(p.name_str), "name"),        //
                        a::path(a::assign_numeric(p.node_numbers), "nodes"),  //
                        a::on_array_element([&](auto const&)
                                            { dest.scenes.emplace_back(std::move(p.name_str), std::move(p.node_numbers)); })  //
                        ),                                                                                                    //
                    "scenes"),                                                                                                //
                a::path(                                                                                                      //
                    a::all(                                                                                                   //
                        a::path(a::assign_string(p.name_str), "name"),                                                        //
                        a::path(a::assign_numeric(p.id1), "mesh"),                                                            //
                        a::path(a::assign_numeric(p.id2), "skin"),                                                            //
                        a::path(a::assign_numeric(p.id4), "camera"),                                                          //
                        a::path(assign_numeric(p.rotation), "rotation"),                                                      //
                        a::path(assign_numeric(p.scale), "scale"),                                                            //
                        a::path(assign_numeric(p.translation), "translation"),                                                //
                        a::path(a::assign_numeric(p.node_numbers), "children"),                                               //
                        a::path(a::assign_numeric(p.u_numbers), "weights"),                                                   //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                dest.nodes.emplace_back(p.id1, p.id2, p.id4, p.rotation, p.scale, p.translation, std::move(p.node_numbers),
                                                        std::move(p.u_numbers), std::move(p.name_str));
                                p.reset_parse_state();
                            })  //
                        ),
                    "nodes"),                                                       //
                a::path(                                                            //
                    a::all(                                                         //
                        a::path(a::assign_string(p.name_str), "name"),              //
                        a::path(                                                    //
                            a::all(                                                 //
                                a::path(a::assign_numeric(p.id1), "sampler"),       //
                                a::path(                                            //
                                    a::all(                                         //
                                        a::path(a::assign_numeric(p.id2), "node"),  //
                                        a::path(resolve_path(p.id4), "path")),      //
                                    "target"),
                                a::on_array_element(
                                    [&](auto const&)
                                    {
                                        p.channels.emplace_back(p.id1, p.id2, static_cast<path_type>(p.id4));
                                        p.reset_ids();
                                    })),
                            "channels"),                                      //
                        a::path(                                              //
                            a::all(                                           //
                                a::path(a::assign_numeric(p.id1), "input"),   //
                                a::path(a::assign_numeric(p.id2), "output"),  //
                                a::path(resolve_animation_style(p.id3_nd), "interpolation"),
                                a::on_array_element(
                                    [&](auto const&)
                                    {
                                        p.samplers.emplace_back(p.id1, p.id2, static_cast<interpolation_type>(p.id3_nd));
                                        p.reset_ids();
                                    })),
                            "samplers"),
                        a::on_array_element(
                            [&](auto const&)
                            {
                                dest.animations.emplace_back(p.name_str, std::move(p.channels), std::move(p.samplers));
                                p.reset_parse_state();
                            })),                                                                                   //
                    "animations"),                                                                                 //
                a::path(                                                                                           //
                    a::all(                                                                                        //
                        a::path(a::assign_string(p.name_str), "name"),                                             //
                        a::path(a::assign_numeric(p.flag1), "doubleSided"),                                        //
                        a::path(assign_numeric(p.translation), "emissiveFactor"),                                  //
                        parse_texture("emissiveTexture", p.emissive_texture),                                      //
                        parse_texture("normalTexture", p.normal_texture),                                          //
                        a::path(a::assign_numeric(p.normal_texture.scale), "normalTexture", "scale"),              //
                        parse_texture("occlusionTexture", p.occlusion_texture),                                    //
                        a::path(a::assign_numeric(p.occlusion_texture.strength), "occlusionTexture", "strength"),  //
                        a::path(resolve_alpha_mode(p.id3_nd), "alphaMode"),                                        //
                        a::path(a::assign_numeric(p.alpha_cut_off), "alphaCutOff"),                                //
                        a::path(                                                                                   //
                            a::all(a::path(assign_numeric(p.color), "base_color_factor"),
                                   a::path(a::assign_numeric(p.fac1), "metallicFactor"),   //
                                   a::path(a::assign_numeric(p.fac2), "roughnessFactor"),  //
                                   parse_texture("baseColorTexture", p.base_color_texture),
                                   parse_texture("metallicRoughnessTexture", p.metallic_roughness_texture)),  //
                            "pbrMetallicRoughness"),                                                          //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                dest.materials.emplace_back(p.name_str,
                                                            pbr_metallic_roughness{p.color, p.base_color_texture, p.fac1, p.fac2, p.metallic_roughness_texture},
                                                            p.normal_texture, p.occlusion_texture, p.emissive_texture, p.translation,
                                                            static_cast<alpha_mode_type>(p.id3_nd), p.alpha_cut_off, p.flag1);
                                p.reset_parse_state();
                            })),                                              //
                    "materials"),                                             //
                a::path(                                                      //
                    a::all(                                                   //
                        a::path(a::assign_string(p.name_str), "name"),        //
                        a::path(a::assign_numeric(p.f_numbers1), "weights"),  //
                        a::path(                                              //
                            a::all(                                           //
                                a::path(                                      //
                                    a::all(                                   //
                                        resolve_attribute(p.id5),
                                        a::assign_numeric(p.id4),  //
                                        [&](auto const& a)
                                        {
                                            p.attribute_data.emplace_back(static_cast<attribute>(p.id5), p.id4);
                                            p.id5 = -1;
                                            p.id4 = -1;
                                        }),
                                    "attributes"),                                //
                                a::path(a::assign_numeric(p.id1), "indices"),     //
                                a::path(a::assign_numeric(p.id2), "material"),    //
                                a::path(a::assign_numeric(p.draw_mode), "mode"),  //
                                a::on_array_element(
                                    [&](auto const&)
                                    {
                                        auto att_flags = static_cast<attribute_flag>(std::accumulate(
                                            p.attribute_data.begin(), p.attribute_data.end(), uint32_t{0},
                                            [](uint32_t fl, auto const& i) { return fl | static_cast<uint32_t>(tiny_tuple::get<0>(i)); }));

                                        p.primitives.emplace_back(std::move(p.attribute_data), p.id1, p.id2,
                                                                  static_cast<mode_type>(p.draw_mode), att_flags);
                                        p.id1 = p.id2 = -1;
                                        p.draw_mode   = 4;
                                    })),    //
                            "primitives"),  //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                dest.meshes.emplace_back(std::move(p.name_str), std::move(p.primitives), std::move(p.f_numbers1));
                                p.reset_parse_state();
                            })),  //
                    "meshes"),
                a::path(                                                            //
                    a::all(                                                         //
                        a::path(a::assign_string(p.name_str), "name"),              //
                        a::path(a::assign_numeric(p.id1), "inverseBindMaterials"),  //
                        a::path(a::assign_numeric(p.id2), "skeleton"),              //
                        a::path(a::assign_numeric(p.u_numbers), "joints"),          //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                dest.skins.emplace_back(p.name_str, p.id2, p.id1, std::move(p.u_numbers));
                                p.reset_parse_state();
                            })),  //
                    "skins"),
                a::path(                                                     //
                    a::all(                                                  //
                        a::path(a::assign_numeric(p.id1), "bufferView"),     //
                        a::path(a::assign_numeric(p.id3_nd), "byteOffset"),  //
                        a::path(a::assign_numeric(p.id2), "count"),          //
                        a::path(a::assign_numeric(p.id4), "componentType"),  //
                        a::path(resolve_type(p.id5), "type"),                //
                        a::path(a::assign_numeric(p.flag1), "normalized"),   //
                        a::path(a::assign_numeric(p.f_numbers1), "max"),     //
                        a::path(a::assign_numeric(p.f_numbers2), "min"),     //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                // TODO error handling component type valid, type string valid, max min same size.. and correct count
                                // i.e. if type is MAT3 - it must be nine
                                dest.accessors.emplace_back(p.id1, p.id3_nd, p.id2, static_cast<component>(p.id4),
                                                            static_cast<attribute_type>(p.id5), p.flag1, std::move(p.f_numbers1),
                                                            std::move(p.f_numbers2));
                                p.reset_parse_state();
                            })),  //
                    "accessors"),
                a::path(                                                  //
                    a::all(                                               //
                        a::path(a::assign_string(p.uri), "uri"),          //
                        a::path(a::assign_string(p.uri), "mime"),         //
                        a::path(a::assign_string(p.name_str), "name"),    //
                        a::path(a::assign_numeric(p.id1), "bufferView"),  //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                if (p.id1 == -1)
                                    dest.images.emplace_back(external_image{std::move(p.name_str), std::move(p.uri)});
                                else
                                    dest.images.emplace_back(infile_image{std::move(p.name_str), std::move(p.uri), p.id1});
                                p.reset_ids();
                                p.uri.clear();
                                p.name_str.clear();
                            })),  //
                    "images"),
                a::path(                                                //
                    a::all(                                             //
                        a::path(a::assign_numeric(p.id1), "sampler"),   //
                        a::path(a::assign_numeric(p.id2), "source"),    //
                        a::path(a::assign_string(p.name_str), "name"),  //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                dest.textures.emplace_back(p.id1, p.id2, std::move(p.name_str));
                                p.reset_ids();
                                p.name_str.clear();
                            })),  //
                    "textures"),
                a::path(                                                 //
                    a::all(                                              //
                        a::path(a::assign_numeric(p.id1), "magFilter"),  //
                        a::path(a::assign_numeric(p.id2), "minFilter"),  //
                        a::path(a::assign_numeric(p.wrap_s), "wrapS"),   //
                        a::path(a::assign_numeric(p.wrap_t), "wrapT"),   //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                // TODO check values
                                dest.samplers.emplace_back(p.id1, p.id2, p.wrap_s, p.wrap_t);
                                p.reset_ids();
                            })),  //
                    "samplers"),
                a::path(                                                     //
                    a::all(                                                  //
                        a::path(a::assign_numeric(p.id1), "buffer"),         //
                        a::path(a::assign_numeric(p.id2), "byteLength"),   //
                        a::path(a::assign_numeric(p.id3_nd), "byteOffset"),  //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                dest.buffer_views.emplace_back(p.id1, p.id2, p.id3_nd);
                                p.reset_ids();
                            })),  //
                    "bufferViews"),
                a::path(                                                  //
                    a::all(                                               //
                        a::path(a::assign_numeric(p.id1), "byteLength"),  //
                        a::path(a::assign_string(p.name_str), "uri"),     //
                        a::on_array_element(
                            [&](auto const&)
                            {
                                if (p.name_str.empty())
                                {
                                    dest.buffers.emplace_back(infile_buffer{static_cast<uint32_t>(p.id1)});
                                    p.reset_ids();
                                }
                                else
                                {
                                    dest.buffers.emplace_back(external_buffer{static_cast<uint32_t>(p.id1), p.name_str});
                                    p.reset_ids();
                                    p.name_str.clear();
                                }
                            })),  //
                    "buffers"))](std::span<char const> const& buf) mutable
    {
        extractor.parse_bytes(std::string_view(buf.data(), buf.size()));
        return parse_state::more_input_needed;
    };
}
