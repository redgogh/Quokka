/**
 * -- Fragment Shader File --
 */
#version 450

layout(location = 0) in vec3 inColor;

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(inColor, 1.0f);
}