#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 projection;
    mat4 view;
} ubo;

layout (location = 0) out vec4 v_color;

void main()
{
    gl_Position = (ubo.projection * /* ubo.view * */ ubo.model) * vec4(a_position, 1.0f);
    v_color = a_color;
}
