#ifndef MAIN_H_
#define MAIN_H_

#include <memory>
#include "driver/render_device.h"
#include "platform/glfw3/window.h"

#include <stdlib.h>
#include <unistd.h>

#ifdef WIN32
#include <direct.h>
#endif

#include <iostream>

#include <stb/stb_image.h>

#include "engine/camera/camera.h"
#include "platform/event/dispatcher.h"
#include "ui/editor/editor.h"
#include <quokka/qk_format.h>
#include <tiny_gltf.h>
#include "engine/objects/render_object.h"

Vertex vertices[] = {
    {{ -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }}, // 左下
    {{  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }}, // 右下
    {{  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }}, // 右上
    {{ -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }}  // 左上
};

uint32_t indices[] = {
    0, 1, 2, // 第一个三角形
    2, 3, 0  // 第二个三角形
};

#endif /* MAIN_H_ */
