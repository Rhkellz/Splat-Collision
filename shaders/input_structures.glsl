// MAY HAVE TO DELETE .spv FILES TO RECOMPILE THIS IF OTHER SHADERS AREN'T MODIFIED!!

#define pi 3.14159

layout(set = 0, binding = 0) uniform Scene_data {
	mat4 view_matrix;
	mat4 proj_matrix;
	vec4 camera_pos;
	vec2 screen_size;
} scene_data;

struct Splat {
    vec3 center;
    float opacity;
    float sh[48];
    float scale[3];
    float rot[4];
	float pad;
};

layout(set = 2, binding = 0) readonly buffer splat_buffer_t { Splat splats[]; } splat_buffer;
layout(set = 1, binding = 0) readonly buffer sorted_indices_t { uint indices[]; } sorted_indices;

layout( push_constant ) uniform constants
{	
	float focal_x;
	float focal_y;
    float min_opacity;
} PushConstants;

// SH constants
const float SH_C0 = 0.28209479177387814;
const float SH_C1 = 0.4886025119029199;
const float SH_C2_0 = 1.0925484305920792;
const float SH_C2_1 = -1.0925484305920792;
const float SH_C2_2 = 0.31539156525252005;
const float SH_C2_3 = -1.0925484305920792;
const float SH_C2_4 = 0.5462742152960396;
const float SH_C3_0 = -0.5900435899266435;
const float SH_C3_1 = 2.890611442640554;
const float SH_C3_2 = -0.4570457994644658;
const float SH_C3_3 = 0.3731763325901154;
const float SH_C3_4 = -0.4570457994644658;
const float SH_C3_5 = 1.445305721320277;
const float SH_C3_6 = -0.5900435899266435;

mat3 quat_to_mat3(float rot[4]) {
    vec4 q = normalize(vec4(rot[0], rot[1], rot[2], rot[3]));
    float w = q.x;
    float x = q.y;
    float y = q.z;
    float z = q.w;

    return mat3(
        1.0 - 2.0 * y * y - 2.0 * z * z,
        2.0 * x * y + 2.0 * w * z,
        2.0 * x * z - 2.0 * w * y,
        2.0 * x * y - 2.0 * w * z,
        1.0 - 2.0 * x * x - 2.0 * z * z,
        2.0 * y * z + 2.0 * w * x,
        2.0 * x * z + 2.0 * w * y,
        2.0 * y * z - 2.0 * w * x,
        1.0 - 2.0 * x * x - 2.0 * y * y);
}


const vec2 quad_corners[4] = vec2[](
    vec2(-1.0, -1.0), 
    vec2( 1.0, -1.0), 
    vec2(-1.0,  1.0), 
    vec2( 1.0,  1.0)
);