/* ==========================================================================
 Copyright (c) 2021 Andreas Pokorny
 Distributed under the Boost Software License, Version 1.0. (See accompanying
 file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
========================================================================== */

#include <async_json/json_extractor.hpp>
#include <async_json/saj_event_value.hpp>
#include <trivial_gltf/gltf_parse.h>
#include <glm/glm.hpp>

namespace trivial_gltf
{
char const* component_names[] = {"SCALAR", "VEC2", "VEC3", "VEC4", "MAT3", "MAT4"};

template <typename T, glm::qualifier Q>
constexpr auto assign_numeric(glm::qua<T, Q>& ref)
{
    return [&ref, index_ctr = 0](auto const& ev) mutable
    {
        switch (ev.value_type())
        {
            case async_json::saj_variant_value::float_number: ref[index_ctr++] = static_cast<T>(ev.as_float_number()); break;
            case async_json::saj_variant_value::number: ref[index_ctr++] = static_cast<T>(ev.as_number()); break;
            default: break;
        }
        if (index_ctr == 4) index_ctr = 0;
    };
}

template <int I, typename T, glm::qualifier Q>
constexpr auto assign_numeric(glm::vec<I, T, Q>& ref)
{
    return [&ref, index_ctr = 0](auto const& ev) mutable
    {
        switch (ev.value_type())
        {
            case async_json::saj_variant_value::float_number: ref[index_ctr++] = static_cast<T>(ev.as_float_number()); break;
            case async_json::saj_variant_value::number: ref[index_ctr++] = static_cast<T>(ev.as_number()); break;
            case async_json::saj_variant_value::boolean: ref[index_ctr++] = static_cast<T>(ev.as_bool()); break;
            // case saj_variant_value::string: number_from_sv_t::try_parse(ev.as_string_view(), ref); break;
            default: break;
        }
        if (index_ctr == I) index_ctr = 0;
    };
}

}  // namespace gltf
