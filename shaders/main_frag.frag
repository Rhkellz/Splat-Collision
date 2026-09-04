//glsl version 4.5
#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"

//shader input
layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 splat_coord;
layout (location = 2) in float splat_opacity;

//output write
layout (location = 0) out vec4 out_frag_color;


void main() {

    float power = -4.5 * dot(splat_coord, splat_coord);
    if (power < -9.0) {
        discard;
    }

    float alpha = min(0.99, splat_opacity * exp(power));
    if (alpha < PushConstants.min_opacity) {
        discard;
    }

    
    out_frag_color = vec4(in_color, alpha);
}
