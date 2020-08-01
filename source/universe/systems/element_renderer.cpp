#include <flame/graphics/canvas.h>
#include "../world_private.h"
#include "../components/element_private.h"
#include "element_renderer_private.h"

namespace flame
{
	void sElementRendererPrivate::do_render(EntityPrivate* e)
	{
		if (!e->global_visibility)
			return;

		auto element = (cElementPrivate*)e->get_component(cElement::type_hash);
		if (!element)
			return;

		auto scissor = canvas->get_scissor();
		element->update_transform();
		//auto r = rect(element->global_pos, element->global_pos + element->global_size);
		//element->clipped = !rect_overlapping(scissor, r);
		//element->clipped_rect = element->clipped ? Vec4f(-1.f) : max(r, scissor);

		if (element->clipping)
		{
			element->draw_background(canvas);
			canvas->set_scissor(Vec4f(element->points[4], element->points[6]));
			element->draw_content(canvas);
			for (auto& c : e->children)
				do_render(c.get());
			canvas->set_scissor(scissor);
		}
		else
		{
			element->draw_background(canvas);
			element->draw_content(canvas);
			for (auto& c : e->children)
				do_render(c.get());
		}
	}

	void sElementRendererPrivate::on_added()
	{
		canvas = (graphics::Canvas*)((WorldPrivate*)world)->find_object("flame::graphics::Canvas");

		auto root = ((WorldPrivate*)world)->root.get();
		if (!root->get_component(cElement::type_hash))
		{
			auto element = (cElementPrivate*)cElementPrivate::create();
			element->fill_color = Vec4c(0);
			root->add_component(element);
		}
	}

	void sElementRendererPrivate::update()
	{
		if (!dirty)
			return;
		do_render(((WorldPrivate*)world)->root.get());
		dirty = false;
	}

	sElementRenderer* sElementRenderer::create()
	{
		return f_new<sElementRendererPrivate>();
	}
}
