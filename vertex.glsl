#version 460

layout (location = 0) in vec3 a_position;


layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 projection;
    mat4 view;
    mat4 model;
    vec3 colour;
} ubo;

layout (location = 0) out vec4 v_color;

#include "util.glsl"

void main()
{
    gl_Position = (ubo.projection * ubo.view * ubo.model) * vec4(a_position, 1.0f);
    vec4 a_color = vec4(ubo.colour,1.0f); // (a_position.z + 0.5f)
    v_color = a_color;
}
