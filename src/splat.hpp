#pragma once

#include <glm/ext/vector_float3.hpp>

#include <array>

constexpr int SH_COUNT = 16;
constexpr int SH_CHANNEL_COUNT = 3;
constexpr int SH_FLOAT_COUNT = SH_COUNT * SH_CHANNEL_COUNT;
constexpr int SH_REST_FLOAT_COUNT = SH_FLOAT_COUNT - SH_CHANNEL_COUNT;
constexpr int SH_PACKED_VEC4_COUNT = SH_FLOAT_COUNT / 4;

static_assert(SH_FLOAT_COUNT % 4 == 0,
    "Spherical harmonic coefficients must pack into vec4 attributes");
static_assert(SH_FLOAT_COUNT == 48,
    "temp requirement, pad later to fix");

struct gaussian_splat {
    glm::vec3 centroid = glm::vec3(0.0f);
    float opacity = 0.0f;
    std::array<float, SH_FLOAT_COUNT> spherical_harmonics = {};
    std::array<float, 3> scale = { 0.0f, 0.0f, 0.0f };
    std::array<float, 4> rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
    float _pad_end = 0.0f;
};
static_assert(sizeof(gaussian_splat) % 16 == 0, "gaussian_splat must be 16-byte aligned for std430");