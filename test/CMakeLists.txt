add_executable(gltf_example
  main.cpp)
target_link_libraries(gltf_example trivial_gltf::gltf async_json::async_json)
