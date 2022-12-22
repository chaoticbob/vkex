/*
 Copyright 2018-2023 Google Inc.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#ifndef __VKEX_SHADERS_COMMON_H__
#define __VKEX_SHADERS_COMMON_H__

#if defined(__cplusplus)
# define GLM_FORCE_RADIANS 
# define GLM_FORCE_DEPTH_ZERO_TO_ONE 
# define GLM_ENABLE_EXPERIMENTAL
# include <glm/glm.hpp>
# include <glm/gtc/matrix_inverse.hpp>
# include <glm/gtc/matrix_transform.hpp>
# include <glm/gtx/euler_angles.hpp>
# include <glm/gtx/matrix_decompose.hpp>
# include <glm/gtx/quaternion.hpp>
# include <glm/gtx/transform.hpp>
#endif

#if defined(__cplusplus)
namespace vkex {
#endif

#if defined(__cplusplus)
// -----------------------------------------------------------------------------
// C++ [begin]
// -----------------------------------------------------------------------------

// Import GLM types as HLSL friendly
// bool
using bool2     = glm::bool2;
using bool3     = glm::bool3;
using bool4     = glm::bool4;

// 32-bit signed integer
using int2      = glm::ivec2;
using int3      = glm::ivec3;
using int4      = glm::ivec4;
// 32-bit unsigned integer
using uint      = glm::uint;
using uint2     = glm::uvec2;
using uint3     = glm::uvec3;
using uint4     = glm::uvec4;

// 32-bit float
using float2    = glm::vec2;
using float3    = glm::vec3;
using float4    = glm::vec4;
// 32-bit float2 matrices
using float2x2  = glm::mat2x2;
using float2x3  = glm::mat2x3;
using float2x4  = glm::mat2x4;
// 32-bit float3 matrices
using float3x2  = glm::mat3x2;
using float3x3  = glm::mat3x3;
using float3x4  = glm::mat3x4;
// 32-bit float4 matrices
using float4x2  = glm::mat4x2;
using float4x3  = glm::mat4x3;
using float4x4  = glm::mat4x4;
// 32-bit float quaternion
using quat      = glm::quat;

// 64-bit float
using double2   = glm::dvec2;
using double3   = glm::dvec3;
using double4   = glm::dvec4;
// 64-bit float2 matrices
using double2x2 = glm::dmat2x2;
using double2x3 = glm::dmat2x3;
using double2x4 = glm::dmat2x4;
// 64-bit float3 matrices
using double3x2 = glm::dmat3x2;
using double3x3 = glm::dmat3x3;
using double3x4 = glm::dmat3x4;
// 64-bit float4 matrices
using double4x2 = glm::dmat4x2;
using double4x3 = glm::dmat4x3;
using double4x4 = glm::dmat4x4;

struct hlsl_float3x3 {
  float4  v0;
  float4  v1;
  float3  v2;
  
  hlsl_float3x3() {}

  hlsl_float3x3(const float3x3& m) 
    : v0(m[0], 0.0f), v1(m[1], 0.0f), v2(m[2]) {}

  hlsl_float3x3& operator=(const float3x3& rhs) {
    v0 = float4(rhs[0], 0.0f);
    v1 = float4(rhs[1], 0.0f);
    v2 = rhs[2];
    return *this;
  }

  operator float3x3() const {
    float3x3 m;
    m[0] = float3(v0);
    m[1] = float3(v1);
    m[2] = v2;
    return m;
  }
};

template <typename T, size_t PadSize>
union hlsl_type {
  T       value;
  uint8_t padded[PadSize];

  hlsl_type& operator=(const T& rhs) {
    value = rhs;
    return *this;
  }
};

#define hlsl_float(PAD_SIZE)    hlsl_type<float, PAD_SIZE>
#define hlsl_float2(PAD_SIZE)   hlsl_type<float2, PAD_SIZE>
#define hlsl_float3(PAD_SIZE)   hlsl_type<float3, PAD_SIZE>
#define hlsl_float4(PAD_SIZE)   hlsl_type<float4, PAD_SIZE>
#define hlsl_float2x2(PAD_SIZE) hlsl_type<float2x2, PAD_SIZE>
#define hlsl_float3x3(PAD_SIZE) hlsl_type<hlsl_float3x3, PAD_SIZE>
#define hlsl_float4x4(PAD_SIZE) hlsl_type<float4x4, PAD_SIZE>

/** Macros
 *
 */
# if defined(_MSC_VER)
#   define VKEX_PACKED_BEGIN __pragma(pack(push,1))
#   define VKEX_PACKED_END   __pragma(pack(pop))
#   define VKEX_PACKED_ATTR  
# else
#   define VKEX_PACKED_BEGIN
#   define VKEX_PACKED_END
#   define VKEX_PACKED_ATTR  __attribute__((packed))
# endif
// -----------------------------------------------------------------------------
// C++ [end]
// -----------------------------------------------------------------------------
#else
// -----------------------------------------------------------------------------
// HLSL [begin]
// -----------------------------------------------------------------------------
#define hlsl_float(PAD_SIZE)    float
#define hlsl_float2(PAD_SIZE)   float2
#define hlsl_float3(PAD_SIZE)   float3
#define hlsl_float4(PAD_SIZE)   float4
#define hlsl_float2x2(PAD_SIZE) float2x2
#define hlsl_float3x3(PAD_SIZE) float3x3
#define hlsl_float4x4(PAD_SIZE) float4x4

#define VKEX_PACKED_BEGIN
#define VKEX_PACKED_END
#define VKEX_PACKED_ATTR
// -----------------------------------------------------------------------------
// HLSL [end]
// -----------------------------------------------------------------------------
#endif

#if defined(__cplusplus) || defined(__STDC__)
# define VKEX_SHADER_CONSTANTS_BASE_REGISTER      0
# define VKEX_SHADER_TEXTURE_BASE_REGISTER        16
# define VKEX_SHADER_SAMPLER_BASE_REGISTER        32
# define VKEX_SHADER_SPACE                        0
// Specialized
# define VKEX_SHADER_VIEW_CONSTANTS_REGISTER      0
# define VKEX_SHADER_MATERIAL_CONSTANTS_REGISTER  1
# define VKEX_SHADER_ALBEDO_TEXTURE_REGISTER      2
#else
# define VKEX_SHADER_CONSTANTS_BASE_REGISTER      b0
# define VKEX_SHADER_TEXTURE_BASE_REGISTER        t16
# define VKEX_SHADER_SAMPLER_BASE_REGISTER        s32
# define VKEX_SHADER_SPACE                        space0
// Specialized
# define VKEX_SHADER_VIEW_CONSTANTS_REGISTER      b0
# define VKEX_SHADER_MATERIAL_CONSTANTS_REGISTER  b1
# define VKEX_SHADER_ALBEDO_TEXTURE_REGISTER      t2
#endif

/** @struct ViewConstantsData
 *
 */
VKEX_PACKED_BEGIN
struct ViewConstantsData 
{
  hlsl_float4x4(64)  M;   // Model Matrix
  hlsl_float4x4(64)  V;   // View Matrix
  hlsl_float4x4(64)  P;   // Projection Matrix
  hlsl_float4x4(64)  MVP; // Model View Projection Matrix
  hlsl_float3x3(48)  N;   // Normal Matrix
  hlsl_float3(16)    EP;  // Eye Position
  hlsl_float3(12)    LP;  // Light Position
} VKEX_PACKED_ATTR;
VKEX_PACKED_END

/** @struct MaterialConstantsData
 *
 */
VKEX_PACKED_BEGIN
struct MaterialConstantsData
{
  hlsl_float3(12) BaseColor;
  hlsl_float(4)   Metallic;
  hlsl_float(4)   Subsurface;
  hlsl_float(4)   Specular;
  hlsl_float(4)   Roughness;
  hlsl_float(4)   SpecularTint;
  hlsl_float(4)   Anisotropic;
  hlsl_float(4)   Sheen;
  hlsl_float(4)   SheenTint;
  hlsl_float(4)   ClearCoat;
  hlsl_float(4)   ClearCoatGloss;
  hlsl_float(4)   kA;
  hlsl_float(4)   kD;
  hlsl_float(4)   kS;
} VKEX_PACKED_ATTR;
VKEX_PACKED_END

#if defined(__cplusplus)
} // namespace vkex
#endif

#endif // __VKEX_SHADERS_COMMON_H__
