# modules/imgui/CMakeLists.txt
include_directories(include/imgui)
add_library(imgui STATIC imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp imgui_demo.cpp
                         backends/imgui_impl_glfw.cpp backends/imgui_impl_vulkan.cpp qk_imgui.cpp)
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)