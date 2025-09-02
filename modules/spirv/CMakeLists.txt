# modules/spirv/CMakeLists.txt
include_directories(include/spirv)
add_library(spirv STATIC spirv_reflect.c)
target_include_directories(spirv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)