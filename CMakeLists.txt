cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_minimum_required(VERSION 3.14)
project(trivial_gltf VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)
fetchcontent_declare(async_json
  GIT_REPOSITORY https://github.com/APokorny/async_json
  GIT_TAG development) # todo: pick a fixed revision

fetchcontent_declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        development) # todo pick a tag

fetchcontent_makeavailable(async_json)

option(gltf_BUILD_TESTS "Build examples and tests" ON)
add_library(gltf 
  include/trivial_gltf/gltf_parse.h
  src/parser.h
  src/parser.cpp)
add_library(trivial_gltf::gltf ALIAS gltf)
target_compile_features(gltf PUBLIC cxx_std_20)
target_link_libraries(gltf PRIVATE async_json)
target_include_directories(gltf INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
    )
install(TARGETS gltf EXPORT gltf-Targets DESTINATION include)
install(EXPORT gltf-Targets
    NAMESPACE gltf::
    DESTINATION lib/cmake/trivial_gltf
    )
install(DIRECTORY include/trivial_gltf DESTINATION include)

if(gltf_BUILD_TESTS)
    fetchcontent_makeavailable(Catch2)
## target_link_libraries(tests Catch2::Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
    include(CTest)
    include(Catch)
    #     catch_discover_tests()
    add_subdirectory(test)
endif()
