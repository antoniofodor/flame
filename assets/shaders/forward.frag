#version 450 core
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in flat uint i_mat_id;
layout (location = 1) in vec2 i_uv;
layout (location = 2) in vec3 i_coordw;
layout (location = 3) in vec3 i_coordv;
layout (location = 4) in vec3 i_normal;

struct MaterialInfo
{
	vec4 color;
	float metallic;
	float roughness;
	float alpha_test;
	float dummy0;
	int color_map_index;
	int metallic_roughness_ao_map_index;
	int normal_height_map_index;
	int dummy1;
};

layout (set = 0, binding = 1) buffer readonly MaterialInfos
{
	MaterialInfo material_infos[];
};

layout (set = 0, binding = 2) uniform sampler2D maps[128];

struct PointLightInfo
{
	vec3 color;
	int dummy0;
	vec3 coord;

	int shadow_map_index;
};

layout (set = 1, binding = 1) buffer readonly PointLightInfos
{
	PointLightInfo point_light_infos[];
};

struct PointLightIndices
{
	uint count;
	uint indices[1023];
};

layout (set = 1, binding = 2) buffer readonly PointLightIndicesList
{
	PointLightIndices point_light_indices_list[];
};

layout (set = 1, binding = 3) uniform samplerCube point_light_shadow_maps[4];

layout (push_constant) uniform PushConstantT
{
	vec2 shadow_map_advance;
}pc;

layout (location = 0) out vec4 o_color;

const float PI = 3.14159265359;

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

	float metallic;
	float roughness;
	if (material.metallic_roughness_ao_map_index >= 0)
	{
		vec4 s = texture(maps[material.metallic_roughness_ao_map_index], i_uv);
		metallic = s.r;
		roughness = s.g;
	}
	else
	{
		metallic = material.metallic;
		roughness = material.roughness;
	}
	vec3 albedo = (1.0 - metallic) * color.rgb;
	vec3 spec = mix(vec3(0.04), color.rgb, metallic);

	vec3 N = normalize(i_normal);
	vec3 V = normalize(i_coordv);
	float NdotV = clamp(dot(N, V), 0.0, 1.0);

	uint count = point_light_indices_list[0].count;
	for (int i = 0; i < count; i++)
	{
		PointLightInfo light = point_light_infos[point_light_indices_list[0].indices[i]];
		float shadow = 1.0;

		vec3 L = light.coord - i_coordw;
		float dist = length(L);
		L = L / dist;

		if (light.shadow_map_index != -1)
		{
			float ref = texture(point_light_shadow_maps[light.shadow_map_index], -L).r;
			//o_color.rgb = vec3(ref / 1000.f);
			//continue;
			if (ref < dist - 0.01)
				continue;
		}

		vec3 Li = light.color / max(dist * dist * 0.01, 1.0);
		Li *= shadow;

		vec3 H = normalize(V + L);
		
		float NdotL = clamp(dot(N, L), 0.0, 1.0);
		float NdotH = clamp(dot(N, H), 0.0, 1.0);
		float LdotH = clamp(dot(L, H), 0.0, 1.0);

		float roughness2 = roughness * roughness;

		float roughness4 = roughness2 * roughness2;
		float denom = NdotH * NdotH *(roughness4 - 1.0) + 1.0;
		float D = roughness4 / (PI * denom * denom);

		float LdotH5 = 1.0 - LdotH;
		LdotH5 = LdotH5*LdotH5*LdotH5*LdotH5*LdotH5;
		vec3 F = spec + (1.0 - spec) * LdotH5;

		float k = roughness2 / 2.0;
		float G = (1.0 / (NdotL * (1.0 - k) + k)) * (1.0 / (NdotV * (1.0 - k) + k));

        vec3 specular = min(F * G * D, vec3(1.0));

		o_color.rgb += ((vec3(1.0) - F) * albedo + specular) * NdotL * Li;
	}

	o_color.a = color.a;
}
