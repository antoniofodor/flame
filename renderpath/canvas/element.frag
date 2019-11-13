in vec4 i_color;
in vec2 i_uv;
in uint i_id;

out vec4 o_color;

void main()
{
	o_color = i_color * texture(images[i_id], i_uv);
}
