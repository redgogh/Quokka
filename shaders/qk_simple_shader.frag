/**
 * -- Fragment Shader File --
 */
#version 450

layout(location = 0) in vec2 uv;

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(tex, uv);
}