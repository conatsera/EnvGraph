

#version 450

layout (std140, binding = 0) readonly uniform _view_parameters {
    mat4 mvp;
    uvec2 window;
    // Allows an environment to draw from multiple cameras with multiple meshes
    vec2 view_offset_meters;
} view_parameters;

layout (binding = 1, r16ui) restrict readonly uniform uimage2D depth_image;
layout (binding = 2, rgba8) restrict readonly uniform image2D color_image;

layout(push_constant) uniform _filter_params {
    vec2 x;
    vec2 y;
    vec2 angle;
} filter_params;

// 3D Position in the virtual space (xyz), scan opacity (w)
layout (location = 0) in vec4 scan_space;
// 2D depth coords (xy), 2D color coords (zw)
layout (location = 1) in uvec4 scan_image_coords;
layout (location = 0) out vec4 color;

mat3 edge_filter_x = {
    {filter_params.x.x, 0.0, filter_params.x.y},
    {filter_params.x.x, 0.0, filter_params.x.y},
    {filter_params.x.x, 0.0, filter_params.x.y}
};

mat3 edge_filter_y = {
    {filter_params.y.x, filter_params.y.x, filter_params.y.x},
    {0.0,               0.0,               0.0              },
    {filter_params.y.y, filter_params.y.y, filter_params.y.y}
};

mat4 edge_filter_angle = {
    {                  0.0, filter_params.angle.x, 0.0, 0.0},
    {filter_params.angle.y, 0.0, filter_params.angle.x, 0.0},
    {0.0, filter_params.angle.y, 0.0, filter_params.angle.x},
    {0.0, 0.0, filter_params.angle.y, 0.0}
};

float Convolve(mat3 filter_kernel, mat3 image_slice) {
    const mat3 c = matrixCompMult(filter_kernel, image_slice);

    const float mag = c[0][0] + c[1][0] + c[2][0]
                    + c[0][1] + c[1][1] + c[2][1]
                    + c[0][2] + c[1][2] + c[2][2];

    return mag;
}

float Convolve(mat4 filter_kernel, mat4 image_slice) {
    const mat4 c = matrixCompMult(filter_kernel, image_slice);

    const float mag = c[0][0] + c[1][0] + c[2][0] + c[3][0]
                    + c[0][1] + c[1][1] + c[2][1] + c[3][1]
                    + c[0][2] + c[1][2] + c[2][2] + c[3][2]
                    + c[0][3] + c[1][3] + c[2][3] + c[3][3];

    return mag;
}

void main() {
    gl_Position = view_parameters.mvp * vec4(scan_space.xyz, 1);

    const vec3 depth_column0 =
          vec3(
              (imageLoad(depth_image, ivec2(scan_image_coords.x - 1, scan_image_coords.y - 1))).x,
              (imageLoad(depth_image, ivec2(scan_image_coords.x - 1, scan_image_coords.y    ))).x,
              (imageLoad(depth_image, ivec2(scan_image_coords.x - 1, scan_image_coords.y + 1))).x
          );

    const vec3 depth_column1 =
          vec3(
              (imageLoad(depth_image, ivec2(scan_image_coords.x, scan_image_coords.y - 1))).x,
              (imageLoad(depth_image, ivec2(scan_image_coords.x, scan_image_coords.y    ))).x,
              (imageLoad(depth_image, ivec2(scan_image_coords.x, scan_image_coords.y + 1))).x
          );

    const vec3 depth_column2 =
          vec3(
              (imageLoad(depth_image, ivec2(scan_image_coords.x + 1, scan_image_coords.y - 1))).x,
              (imageLoad(depth_image, ivec2(scan_image_coords.x + 1, scan_image_coords.y    ))).x,
              (imageLoad(depth_image, ivec2(scan_image_coords.x + 1, scan_image_coords.y + 1))).x
          );

    const mat3 depth_slice = mat3(depth_column0, depth_column1, depth_column2);

    const float edge_x = Convolve(edge_filter_x, depth_slice);

    const float edge_y = Convolve(edge_filter_y, depth_slice);

    //const float edge_a = FilterA(depth_color);
}