#ifndef HOST_DEVICE
#define HOST_DEVICE

#ifdef __cplusplus
#include <glm/glm.hpp>

// glsl types
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

#include "constants.h"

struct BoneUBO
{
	mat4 bone_matrices[MAX_BONE_NUM];
};

struct TransformPCO
{
    mat4 m;
    mat4 nm;
    mat4 mvp;
};

struct MaterialPCO
{
    vec4 base_color_factor;
    vec4 emissive_factor;
	float m_metallic_factor;
	float m_roughness_factor;

    int has_base_color_texture; 
    int has_emissive_texture;
    int has_metallic_roughness_occlusion_texture;
    int contains_occlusion_channel;
    int has_normal_texture;
};

struct SkyLight
{
    vec3 color;
	float prefilter_mip_levels;
};

struct DirectionalLight
{
	vec3 direction; float padding0;
	vec3 color; float padding1;
};

struct PointLight
{
	vec3 position; 
    float padding0; // inner_cutoff for SpotLight

	vec3 color; 
    float padding1; // outer_cutoff for SpotLight

    float radius;
	float linear_attenuation;
	float quadratic_attenuation;
    float padding2;
};

struct SpotLight
{
    PointLight _pl;
    vec3 direction; float padding0;
};

struct LightingUBO
{
    // camera
    vec3 camera_pos; float padding0;
    mat4 inv_view_proj;

    // lights
    SkyLight sky_light;
    DirectionalLight directional_light;
    PointLight point_lights[MAX_POINT_LIGHT_NUM];
    SpotLight spot_lights[MAX_SPOT_LIGHT_NUM];

    int has_sky_light;
    int has_directional_light;
    int point_light_num;
    int spot_light_num;
};

struct MaterialInfo
{
    vec3 position;
    vec3 normal;
    vec4 base_color;
    vec4 emissive_color;
    float metallic;
    float roughness;
    float occlusion;
};

#endif