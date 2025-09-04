/**
 * -- Vertex Shader File --
 */
#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 outUV;

layout(set = 0, binding = 0) uniform Camera {
    mat4 mvp;
} camera;

void main()
{
    gl_Position = camera.mvp * vec4(pos, 0.0f, 1.0f);
    outUV = uv;
}