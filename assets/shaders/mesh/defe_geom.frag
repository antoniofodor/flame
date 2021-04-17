#include "defe_geom.pll"

layout (location = 0) in flat uint i_mat;
layout (location = 1) in vec2 i_uv;
layout (location = 2) in vec3 i_normal;

layout (location = 0) out vec4 o_res_alb_met;
layout (location = 1) out vec4 o_res_nor_rou;

void main()
{
	MaterialInfo material = material_infos[i_mat];

	vec3 N = normalize(i_normal);
	//vec3 V = normalize(i_coordv);
	
	MAT_FILE

	o_res_alb_met = vec4(albedo, metallic);
	o_res_nor_rou = vec4(i_normal * 0.5 + vec3(0.5), roughness);
}
