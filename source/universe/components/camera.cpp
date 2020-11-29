#include <flame/graphics/image.h>
#include <flame/graphics/canvas.h>
#include "node_private.h"
#include "camera_private.h"
#include "../systems/renderer_private.h"

namespace flame
{
	void cCameraPrivate::apply_current()
	{
		if (current)
		{
			if (renderer->camera != this)
			{
				if (renderer->camera)
					renderer->camera->current = false;
				renderer->camera = this;
			}
		}
		else
		{
			if (renderer->camera == this);
			renderer->camera = nullptr;
		}
	}

	void cCameraPrivate::on_gain_renderer()
	{
		apply_current();
	}

	void cCameraPrivate::on_lost_renderer()
	{
		current = false;
		apply_current();
	}

	void cCameraPrivate::draw(graphics::Canvas* canvas)
	{
		auto out = canvas->get_output(0);
		auto size = out ? vec2(out->get_image()->get_size()) : vec2(1.f);
		canvas->set_camera(fovy, size.x / size.y, near, far, node->g_rot, node->g_pos);
	}

	void cCameraPrivate::set_current(bool v)
	{
		if (current == v)
			return;
		current = v;

		if (renderer)
			apply_current();
	}

	cCamera* cCamera::create()
	{
		return f_new<cCameraPrivate>();
	}
}
