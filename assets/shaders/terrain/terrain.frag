#include "../math.glsl"

#ifndef GBUFFER_PASS
#ifdef MAT_CODE
#include "../shading.glsl"
#endif
#endif

layout(location = 0) in flat uint i_id;
layout(location = 1) in flat uint i_matid;
layout(location = 2) in		 vec2 i_uv;
#ifndef OCCLUDER_PASS
layout(location = 3) in		 vec3 i_normal;
layout(location = 4) in		 vec3 i_tangent;
layout(location = 5) in		 vec3 i_coordw;
#endif

#ifndef OCCLUDER_PASS
#ifndef GBUFFER_PASS
layout(location = 0) out vec4 o_color;
#else
layout(location = 0) out vec4 o_res_col_met;
layout(location = 1) out vec4 o_res_nor_rou;
#endif

#ifdef MAT_CODE
vec3 textureVariant(int map_id, vec2 uv)
{
	float k = 0;
	if (RAND_TEX_ID != -1)
		k = sample_map(RAND_TEX_ID, 0.005 * uv).r;

    vec2 duvdx = dFdx(uv);
    vec2 duvdy = dFdy(uv);
    
    float l = k * 8.0;
    float f = fract(l);

    float ia = floor(l);
    float ib = ia + 1.0;
    
    vec2 offa = sin(vec2(3.0, 7.0) * ia);
    vec2 offb = sin(vec2(3.0, 7.0) * ib);

    vec3 cola = textureGrad(material_maps[map_id], uv + offa, duvdx, duvdy).rgb;
    vec3 colb = textureGrad(material_maps[map_id], uv + offb, duvdx, duvdy).rgb;
    return mix(cola, colb, smoothstep(0.2, 0.8, f - 0.1 * sum(cola - colb)));
}

vec3 textureTerrain(int map_id, float tiling)
{
    vec3 ret = vec3(0.0);
    #ifdef TRI_PLANAR
        vec3 blending = abs(i_normal);
        blending = normalize(max(blending, 0.00001));
        blending /= blending.x + blending.y + blending.z;
        if (blending.x > 0)
            ret += textureVariant(map_id, i_coordw.yz * tiling) * blending.x;
        if (blending.y > 0)
            ret += textureVariant(map_id, i_coordw.xz * tiling) * blending.y;
        if (blending.z > 0)
            ret += textureVariant(map_id, i_coordw.xy * tiling) * blending.z;
    #else
        ret += textureVariant(map_id, i_uv * tiling);
    #endif
    return ret;
}
#endif

#endif

void main()
{
#ifdef MAT_CODE
	MaterialInfo material = material.infos[i_matid];
	#include MAT_CODE
#else
	#ifndef OCCLUDER_PASS
		#ifndef GBUFFER_PASS
			#ifdef PICKUP
				o_color = pack_uint_to_v4(pc.i[0]);
			#elifdef NORMAL_DATA
				o_color = vec4(i_normal * 0.5 + vec3(0.5), 1.0);
			#else
				o_color = pc.f;
			#endif
		#else
			o_res_col_met = vec4(1.0, 1.0, 1.0, 0.0);
			o_res_nor_rou = vec4(i_normal * 0.5 + 0.5, 1.0);
		#endif
	#endif
#endif
}
