#version 450 core
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in flat uint i_mat_id;
layout (location = 1) in vec3 i_coordw;
layout (location = 2) in vec3 i_coordv;
layout (location = 3) in vec3 i_normal;
layout (location = 4) in vec2 i_uv;

struct LightInfo
{
	vec4 col;
	vec4 pos;
};

layout (set = 0, binding = 2) buffer readonly LightInfos
{
	LightInfo light_infos[];
};

struct LightIndices
{
	uint count;
	uint indices[1023];
};

layout (set = 0, binding = 3) buffer readonly LightIndicesList
{
	LightIndices light_indices_list[];
};

struct MaterialInfo
{
	vec4 color;
	float metallic;
	float roughness;
	float depth;
	float alpha_test;
	int color_map_index;
	int normal_roughness_map_index;
};

layout (set = 0, binding = 4) uniform MaterialInfos
{
	MaterialInfo material_infos[128];
};

layout (set = 0, binding = 5) uniform sampler2D maps[128];

layout (location = 0) out vec4 o_color;

const float PI = 3.14159265359;

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
	return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float geometry_smith(float NdotV, float NdotL, float roughness)
{
    float g2  = geometry_schlick_ggx(NdotV, roughness);
    float g1  = geometry_schlick_ggx(NdotL, roughness);
	
    return g1 * g2;
}

float distribution_ggx(float NdotH, float roughness)
{
	float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

void main()
{
	o_color = vec4(0);

	MaterialInfo material = material_infos[i_mat_id];

	vec4 color;
	if (material.color_map_index >= 0)
		color = texture(maps[material.color_map_index], i_uv);
	else
		color = material.color;

	if (color.a < material.alpha_test)
		discard;

	vec3 albedo = (1.0 - material.metallic) * color.rgb;
	vec3 spec = mix(vec3(0.04), color.rgb, material.metallic);
	float roughness = material.roughness;

	vec3 N = normalize(i_normal);
	vec3 V = normalize(i_coordv);

	uint count = light_indices_list[0].count;
	for (int i = 0; i < count; i++)
	{
		LightInfo light = light_infos[light_indices_list[0].indices[i]];

		vec3 L = light.pos.xyz - i_coordw * light.pos.w;
		float dist = length(L);
		L = L / dist;

		vec3 H = normalize(V + L);
		
		float NdotL = max(dot(N, L), 0.0);
		float NdotV = max(dot(N, V), 0.0);
		float NdotH = max(dot(N, H), 0.0);

		vec3 radiance = NdotL * light.col.rgb / (dist * dist * 0.01);

		vec3 F = fresnel_schlick(max(dot(H, V), 0.0), spec);
		float G = geometry_smith(NdotV, NdotL, roughness);;
		float D = distribution_ggx(NdotH, roughness);

		vec3 numerator    = F * G * D;
        float denominator = 4.0 * NdotV * NdotL;
        vec3 specular     = numerator / max(denominator, 0.001);
		
		o_color.rgb += ((vec3(1.0) - F) * albedo / PI + specular) * radiance;
	}

	o_color.a = color.a;
}
