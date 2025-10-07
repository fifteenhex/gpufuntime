#version 460

layout (location = 0) in vec3 a_position;
//layout (location = 1) in vec4 a_color;
//layout (location = 2) in float rot;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 projection;
    mat4 view;
    float rot;
} ubo;

layout (location = 0) out vec4 v_color;

#include "util.glsl"

void main()
{
    //vec4 a_color = vec4(a_position.xyz, 1.5f);
    mat4 rotmat = rot_y(ubo.rot) * rot_z(0.3f);
    gl_Position = (ubo.projection * /* ubo.view * */ rotmat) * vec4(a_position, 1.0f);
    vec4 a_color = vec4(vec3(0.f), (a_position.z + 0.5f) * .5f);
    v_color = a_color;
}
