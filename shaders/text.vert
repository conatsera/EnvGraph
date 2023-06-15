#version 460

layout(location = 0) in vec2 v_position;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec2 v_uv;

layout(location = 3) in vec3 i_position;
layout(location = 4) in vec4 i_rotation_quat;
layout(location = 5) in vec3 i_scale;
layout(location = 6) in int i_texture_index;

layout(binding = 0) readonly uniform UBO {
    mat4 mvp;
} ubo;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec2 o_uv;

void main() {
    gl_Position = ubo.mvp * vec4(
        vec3(
            v_position.x * i_scale.x + i_position.x,
            v_position.y * i_scale.y + i_position.y,
            i_scale.z
        ),
        1.0
    );
    o_color = v_color;
    o_uv = v_uv;
}