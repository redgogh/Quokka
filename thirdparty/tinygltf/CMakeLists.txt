# modules/tinygltf/CMakeLists.txt
include_directories(include)
add_library(tinygltf STATIC tiny_gltf.cc)
target_link_libraries(tinygltf PUBLIC stb)
target_include_directories(tinygltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)