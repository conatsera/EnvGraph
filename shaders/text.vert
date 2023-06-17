#version 460

layout(location = 0) in vec2 v_position;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec2 v_uv;

layout(set = 0, binding = 0) readonly uniform UBO {
    mat4 mvp;
} ubo;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec2 o_uv;

void main() {
    gl_Position = ubo.mvp * vec4(
        v_position.x,
        v_position.y,
        0.0,
        1.0
    );
    o_color = v_color;
    o_uv = v_uv;
}
