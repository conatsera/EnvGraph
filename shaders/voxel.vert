#version 450

layout (std140, binding = 0) readonly uniform ViewState
{
    mat4 mvp;
} viewstate;

layout(std430, binding = 1) readonly uniform Light
{
    float brightness;
    vec3 position;
    vec4 orientation;
};

layout(binding = 1, location=0) Light light1;
layout(binding = 1, location=1) Light light2;
layout(binding = 1, location=2) Light light3;
layout(binding = 1, location=3) Light light4;

layout (location = 0) in vec4 voxelVert;
layout (location = 0) out vec4 voxelColor;

void main()
{
    gl_Position = viewstate.mvp * voxelVert;
}