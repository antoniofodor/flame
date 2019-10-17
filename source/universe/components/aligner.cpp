#include <flame/universe/components/element.h>
#include <flame/universe/components/aligner.h>

namespace flame
{
	struct cAlignerPrivate: cAligner
	{
		cAlignerPrivate()
		{
			element = nullptr;

			x_align = AlignxFree;
			y_align = AlignyFree;
			min_size = -1.f;
			width_policy = SizeFixed;
			width_factor = 1.f;
			height_policy = SizeFixed;
			height_factor = 1.f;
			using_padding = false;
		}

		virtual void on_component_added(Component* c) override
		{
			if (c->type_hash == cH("Element"))
				element = (cElement*)c;
		}

		virtual void update() override
		{
			if (min_size.x() < 0.f && width_policy == SizeGreedy)
				min_size.x() = element->size.x();
			if (min_size.y() < 0.f && height_policy == SizeGreedy)
				min_size.y() = element->size.y();
		}

		virtual Component* copy() override
		{
			auto copy = new cAlignerPrivate();

			copy->x_align = x_align;
			copy->y_align = y_align;
			copy->min_size = min_size;
			copy->width_policy = width_policy;
			copy->width_factor = width_factor;
			copy->height_policy = height_policy;
			copy->height_factor = height_factor;
			copy->using_padding = using_padding;

			return copy;
		}
	};

	cAligner* cAligner::create()
	{
		return new cAlignerPrivate();
	}
}
