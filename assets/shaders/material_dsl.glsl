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

layout (set = 1, binding = 0) buffer readonly MaterialInfos
{
	MaterialInfo material_infos[];
};

layout (set = 1, binding = 1) uniform sampler2D maps[128];
