#include <fstream>
#include <cstring>
#include <iostream>
#include <async_json/json_extractor.hpp>
#include <trivial_gltf/gltf_parse.h>

namespace a = async_json;
void parse_json(std::istream& in, std::size_t length)
{
    std::string buf;
    std::string foo;
    buf.resize(length);
    in.read(buf.data(), length);
    trivial_gltf::doc dest;

    auto parser = trivial_gltf::create_parser(dest);

    parser(std::string_view(buf));

    for (auto const& item : dest.nodes) std::cout << "node: " << item.name << '\n';
    for (auto const& item : dest.meshes) std::cout << "mesh: " << item.name << '\n';
    for (auto const& item : dest.textures) std::cout << "texture: " << item.name << '\n';
    for (auto const& item : dest.animations)
    {
        std::cout << "animation: " << item.name << " ";
        for (auto const& c : item.channels) std::cout << "CH:" << c.sampler_id << " " << c.node_id << ", ";
        for (auto const& s : item.samplers) std::cout << "SA:" << s.input << " " << s.output << ", ";
        std::cout << "\n";
    }
    for (auto const& item : dest.buffer_views)
        std::cout << "buffer: " << item.buffer << " " << item.length << " " << item.offset << " " << item.stride << " " << item.target
                  << '\n';
    for (auto const& item : dest.accessors)
        std::cout << "accessor: v:" << item.view << " o:" << item.offset << " c:" << item.count << " " << static_cast<int>(item.comp_type)
                  << " " << static_cast<int>(item.type) << '\n';
}

int main(int argc, char const** argv)
{
    if (argc == 2)
    {
        std::ifstream input(argv[1], std::ios::binary | std::ios::in);

        char header[12];
        input.read(header, 12);
        if (header[0] == 'g' && header[1] == 'l' && header[2] == 'T' && header[3] == 'F')
        {
            uint32_t version;
            uint32_t length;
            std::memcpy(&version, header + 4, 4);
            std::memcpy(&length, header + 8, 4);
            std::cout << "GLTF Found\n" << version << "  " << length << '\n';
            char chunk_data[8];
            input.read(chunk_data, 8);
            uint32_t chunk_length;
            char     chunk_type[5] = {0};
            std::memcpy(&chunk_length, chunk_data, 4);
            std::memcpy(&chunk_type, chunk_data + 4, 4);
            std::cout << "GLTF Found\n" << chunk_length << "  " << chunk_type << '\n';
            // parse json
            parse_json(input, chunk_length);

            input.seekg(chunk_length + ((chunk_length & 3) != 0) * 4 + sizeof(header));
        }
    }
}
