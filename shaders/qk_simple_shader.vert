/**
 * -- Vertex Shader File --
 */
#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

void main()
{
    gl_Position = pc.mvp * vec4(pos, 0.0f, 1.0f);
    outColor = color;
    outUV = uv;
}