CMAKE_MINIMUM_REQUIRED(VERSION 3.28.1...3.31)
PROJECT(Quokka)

SET(CMAKE_CXX_STANDARD 26)

ADD_DEFINITIONS(
  -DIMGUI_IMPL_VULKAN_USE_VOLK
)

INCLUDE_DIRECTORIES(./)
INCLUDE_DIRECTORIES(SYSTEM "thirdparty" "include")

ADD_SUBDIRECTORY("thirdparty/volk")
ADD_SUBDIRECTORY("thirdparty/stb")
ADD_SUBDIRECTORY("thirdparty/imgui")
ADD_SUBDIRECTORY("thirdparty/spirv")
ADD_SUBDIRECTORY("thirdparty/tinygltf")

FILE(GLOB_RECURSE SOURCES
  "driver/*.cpp"
  "platform/*.cpp"
  "engine/*.cpp"
  "ui/*.cpp"
  "event/*.cpp"
  "main.cpp"
)

IF (APPLE)
    LINK_DIRECTORIES("thirdparty/GLFW/libs/macOS/lib-x86_64")
ENDIF()

IF (WIN32)
    LINK_DIRECTORIES("thirdparty/GLFW/libs/win32/lib-mingw-w64")
ENDIF()

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE
  "volk"
  "stb"
  "glfw3"
  "imgui"
  "spirv"
  "tinygltf"
)

IF (APPLE)
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE
    "-framework Cocoa"
    "-framework CoreFoundation"
    "-framework IOKit"
    "-framework CoreVideo"
  )
ENDIF()