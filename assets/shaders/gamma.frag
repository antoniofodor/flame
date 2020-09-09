#version 450 core
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 i_uv;

layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D image;

void main()
{
	float gamma = 2.2;
	o_color = vec4(pow(texture(image, i_uv).rgb, vec3(1.0 / gamma)), 1.0);
}
