#include "sky.pll"

layout (location = 0) in vec3 i_position;
layout (location = 2) in vec3 i_normal;

layout (location = 0) out vec3 o_dir;

void main()
{
	o_dir = -i_normal;
	gl_Position = render_data.proj_view * vec4(i_position, 1.0);
}
