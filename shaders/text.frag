#version 460

layout(set = 0, binding = 0) uniform sampler2D s_font_texture;

layout(location = 0) in vec4 i_color;
layout(location = 1) in vec2 i_uv;

layout(location = 0) out vec4 f_color;

void main() {
    f_color = i_color * texture(s_font_texture, i_uv.st);
}