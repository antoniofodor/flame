#include <flame/graphics/canvas.h>
#include "../entity_private.h"
#include "element_private.h"
#include "blur_private.h"

namespace flame
{
	void cBlurPrivate::set_radius(uint r)
	{
		if (radius == r)
			return;
		radius = r;
		if (element)
			element->mark_drawing_dirty();
		Entity::report_data_changed(this, S<ch("radius")>::v);
	}

	void cBlurPrivate::draw0(graphics::Canvas* canvas)
	{
		canvas->add_blur(element->aabb, radius);
	}

	cBlur* cBlur::create()
	{
		return f_new<cBlurPrivate>();
	}
}
