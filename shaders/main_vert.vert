#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 splat_coord;
layout (location = 2) out float splat_opacity;

float SH_data(int index, int i) {
    return splat_buffer.splats[i].sh[index];
}

vec3 SH_coeff(int coefficient, int i) {
    int base_index = coefficient * 3;
    return vec3(SH_data(base_index, i), SH_data(base_index + 1, i), SH_data(base_index + 2, i));
}

vec3 SH_to_RGB(vec3 direction, int i) {
    float x = direction.x;
    float y = direction.y;
    float z = direction.z;
    vec3 rgb = SH_C0 * SH_coeff(0, i);
    rgb += -SH_C1 * y * SH_coeff(1, i);
    rgb += SH_C1 * z * SH_coeff(2, i);
    rgb += -SH_C1 * x * SH_coeff(3, i);
    rgb += SH_C2_0 * x * y * SH_coeff(4, i);
    rgb += SH_C2_1 * y * z * SH_coeff(5, i);
    rgb += SH_C2_2 * (2.0 * z * z - x * x - y * y) * SH_coeff(6, i);
    rgb += SH_C2_3 * x * z * SH_coeff(7, i);
    rgb += SH_C2_4 * (x * x - y * y) * SH_coeff(8, i);
    rgb += SH_C3_0 * y * (3.0 * x * x - y * y) * SH_coeff(9, i);
    rgb += SH_C3_1 * x * y * z * SH_coeff(10, i);
    rgb += SH_C3_2 * y * (4.0 * z * z - x * x - y * y) * SH_coeff(11, i);
    rgb += SH_C3_3 * z * (2.0 * z * z - 3.0 * x * x - 3.0 * y * y) * SH_coeff(12, i);
    rgb += SH_C3_4 * x * (4.0 * z * z - x * x - y * y) * SH_coeff(13, i);
    rgb += SH_C3_5 * z * (x * x - y * y) * SH_coeff(14, i);
    rgb += SH_C3_6 * x * (x * x - 3.0 * y * y) * SH_coeff(15, i);
    return clamp(rgb + vec3(0.5), 0.0, 1.0);
}

mat3 reconstruct_cov(float scale[3], float rot[4]) {
    // Exponentiate log-space scales on GPU
    vec3 sigma = vec3(scale[0], scale[1], scale[2]);
    
    mat3 r = quat_to_mat3(rot);
    mat3 s = mat3(sigma.x, 0, 0, 0, sigma.y, 0, 0, 0, sigma.z);
    mat3 m = r * s;
    mat3 covariance = m * transpose(m);

    // Y-flip alignment to match standard PLY conventions
    mat3 previewFlipY = mat3(
        1.0,  0.0, 0.0,
        0.0, -1.0, 0.0,
        0.0,  0.0, 1.0
    );
    return previewFlipY * covariance * previewFlipY;
}

mat3 computeJacobian(vec3 view_space_pos, float focal_x, float focal_y) {
    float x = view_space_pos.x;
    float y = view_space_pos.y;
    float z = view_space_pos.z;
    float inv_z = 1.0 / z;
    float inv_z2 = inv_z * inv_z;

    return transpose(mat3(
        vec3(focal_x * inv_z, 0.0, -focal_x * x * inv_z2), 
        vec3(0.0, focal_y * inv_z, -focal_y * y * inv_z2), 
        vec3(0.0, 0.0, 0.0)                               
    ));
}

void main() 
{   
    int splat_index = int(sorted_indices.indices[gl_InstanceIndex]);
    vec3 splat_world_pos = splat_buffer.splats[splat_index].center;

    vec4 clip_pos = scene_data.proj_matrix * scene_data.view_matrix * vec4(splat_world_pos, 1.0f);
    vec4 view_pos = scene_data.view_matrix * vec4(splat_world_pos, 1.0f);

    if (clip_pos.w <= 0.0 || view_pos.z >= -0.001 || scene_data.screen_size.x <= 0.0 || scene_data.screen_size.y <= 0.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        splat_coord = quad_corners[gl_VertexIndex];
        out_color = vec3(0.0);
        splat_opacity = 0.0;
        return;
    }

    vec2 quad_corner = quad_corners[gl_VertexIndex];

    vec3 direction = normalize(splat_world_pos - scene_data.camera_pos.xyz);
    out_color = SH_to_RGB(direction, splat_index);

    mat3 cov = reconstruct_cov(splat_buffer.splats[splat_index].scale, splat_buffer.splats[splat_index].rot);
    mat3 cov_prime = mat3(scene_data.view_matrix) * cov * transpose(mat3(scene_data.view_matrix));
    float focalX = scene_data.proj_matrix[0][0] * scene_data.screen_size.x * 0.5;
    float focalY = scene_data.proj_matrix[1][1] * scene_data.screen_size.y * 0.5;
    mat3 J = computeJacobian(vec3(view_pos), focalX, focalY);
    mat3 cov_2D_full = J * cov_prime * transpose(J);
    mat2 cov_2D = mat2(cov_2D_full);

    float a = cov_2D[0][0] + 0.3;
    float b = cov_2D[1][0];
    float d = cov_2D[1][1] + 0.3;

    float det = a * d - b * b;
    float mid = 0.5 * (a + d);
    float radius = sqrt(max(mid * mid - det, 0.0));
    float lam1 = max(mid + radius, 0.01);
    float lam2 = max(mid - radius, 0.01);
    vec2 e1 = abs(b) > 0.00001 ? normalize(vec2(b, lam1 - a))
                               : (a >= d ? vec2(1.0, 0.0) : vec2(0.0, 1.0));
    vec2 e2 = vec2(-e1.y, e1.x);

    vec2 b1 = 3.0 * sqrt(lam1) * e1;
    vec2 b2 = 3.0 * sqrt(lam2) * e2;

    vec3 ndc = clip_pos.xyz / clip_pos.w;

    splat_coord = quad_corner;
    float opacity = splat_buffer.splats[splat_index].opacity;
    splat_opacity = 1.0 / (1.0 + exp(-opacity));

    vec2 deltaPixel = quad_corner.x * b1 + quad_corner.y * b2;
    vec2 deltaNdc = deltaPixel / (scene_data.screen_size * 0.5);

    gl_Position = vec4(ndc.xy + deltaNdc, ndc.z, 1.0);
}