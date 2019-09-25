#include <flame/universe/components/element.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/layout.h>

namespace flame
{
	struct cLayoutPrivate : cLayout
	{
		cLayoutPrivate(LayoutType$ _type)
		{
			element = nullptr;

			type = _type;
			item_padding = 0.f;
			width_fit_children = true;
			height_fit_children = true;
			fence = -1;
			scroll_offset = Vec2f(0.f);

			content_size = Vec2f(0.f);
		}

		void apply_h_free_layout(const std::pair<cElement*, cAligner*>& al, bool lock = false)
		{
			auto padding = (lock || (al.second ? al.second->using_padding : false)) ? element->inner_padding.xz() : Vec2f(0.f);
			auto w = element->size.x() - padding[0] - padding[1];
			switch (al.second ? al.second->width_policy : SizeFixed)
			{
			case SizeFitParent:
				al.first->size.x() = w;
				break;
			case SizeGreedy:
				if (w > al.second->min_size.x())
					al.first->size.x() = w;
				break;
			}
			switch (al.second ? al.second->x_align : AlignxFree)
			{
			case AlignxFree:
				if (!lock)
					break;
			case AlignxLeft:
				al.first->pos.x() = scroll_offset.x() + padding[0];
				break;
			case AlignxMiddle:
				al.first->pos.x() = scroll_offset.x() + (w - al.first->size.x()) * 0.5f;
				break;
			case AlignxRight:
				al.first->pos.x() = scroll_offset.x() + element->size.x() - padding[1] - al.first->size.x();
				break;
			}
		}

		void apply_v_free_layout(const std::pair<cElement*, cAligner*>& al, bool lock = false)
		{
			auto padding = (lock || (al.second ? al.second->using_padding : false)) ? element->inner_padding.yw() : Vec2f(0.f);
			auto h = element->size.y() - padding[0] - padding[1];
			switch (al.second ? al.second->height_policy : SizeFixed)
			{
			case SizeFitParent:
				al.first->size.y() = h;
				break;
			case SizeGreedy:
				if (h > al.second->min_size.y())
					al.first->size.y() = h;
				break;
			}
			switch (al.second ? al.second->y_align : AlignyFree)
			{
			case AlignyFree:
				if (!lock)
					break;
			case AlignyTop:
				al.first->pos.y() = scroll_offset.y() + padding[0];
				break;
			case AlignyMiddle:
				al.first->pos.y() = scroll_offset.y() + (h - al.first->size.y()) * 0.5f;
				break;
			case AlignyBottom:
				al.first->pos.y() = scroll_offset.y() + element->size.y() - padding[1] - al.first->size.y();
				break;
			}
		}

		void start()
		{
			element = (cElement*)(entity->find_component(cH("Element")));
			assert(element);
			aligner = (cAligner*)(entity->find_component(cH("Aligner")));
		}

		void update()
		{
			std::vector<std::pair<cElement*, cAligner*>> als;
			for (auto i = 0; i < entity->child_count(); i++)
			{
				auto e = entity->child(i);
				if (e->global_visible)
				{
					auto al = (cAligner*)e->find_component(cH("Aligner"));
					als.emplace_back(al ? al->element : (cElement*)entity->child(i)->find_component(cH("Element")), al);
				}
				else
					int cut = 1;
			}

			switch (type)
			{
			case LayoutFree:
				for (auto al : als)
				{
					apply_h_free_layout(al);
					apply_v_free_layout(al);
				}
				break;
			case LayoutHorizontal:
			{
				auto n = min(fence, (uint)als.size());

				auto w = 0.f;
				auto h = 0.f;
				auto factor = 0.f;
				for (auto i = 0; i < n; i++)
				{
					auto& al = als[i];

					switch (al.second ? al.second->width_policy : SizeFixed)
					{
					case SizeFixed:
						w += al.first->size.x();
						break;
					case SizeFitParent:
						factor += al.second->width_factor;
						break;
					case SizeGreedy:
						factor += al.second->width_factor;
						w += al.second->min_size.x();
						break;
					}
					switch (al.second ? al.second->height_policy : SizeFixed)
					{
					case SizeFixed:
						h = max(al.first->size.y(), h);
						break;
					case SizeFitParent:
						break;
					case SizeGreedy:
						h = max(al.second->min_size.y(), h);
						break;
					}
					w += item_padding;
				}
				if (fence > 0 && !als.empty())
					w -= item_padding;
				content_size = Vec2f(w, h);
				w += element->inner_padding_horizontal();
				h += element->inner_padding_vertical();

				if (width_fit_children)
				{
					if (aligner && aligner->width_policy == SizeGreedy)
					{
						aligner->min_size.x() = w;
						element->size.x() = max(element->size.x(), w);
					}
					else
						element->size.x() = w;
				}
				w = element->size.x() - w;
				if (w > 0.f && factor > 0)
					w /= factor;
				else
					w = 0.f;
				auto x = element->inner_padding[0];
				for (auto i = 0; i < n; i++)
				{
					auto& al = als[i];

					if (al.second)
					{
						if (al.second->width_policy == SizeFitParent)
							al.first->size.x() = w * al.second->width_factor;
						else if (al.second->width_policy == SizeGreedy)
							al.first->size.x() = al.second->min_size.x() + w * al.second->width_factor;
					}
					assert(!al.second || al.second->x_align == AlignxFree);
					al.first->pos.x() = scroll_offset.x() + x;
					x += al.first->size.x() + item_padding;
				}
				if (height_fit_children)
				{
					if (aligner && aligner->height_policy == SizeGreedy)
					{
						aligner->min_size.y() = h;
						element->size.y() = max(element->size.y(), h);
					}
					else
						element->size.y() = h;
				}
				for (auto i = n; i < als.size(); i++)
					apply_h_free_layout(als[i], false);
				for (auto i = 0; i < n; i++)
					apply_v_free_layout(als[i], true);
				for (auto i = n; i < als.size(); i++)
					apply_v_free_layout(als[i], false);
			}
				break;
			case LayoutVertical:
			{
				auto n = min(fence, (uint)als.size());

				auto w = 0.f;
				auto h = 0.f;
				auto factor = 0.f;
				for (auto i = 0; i < n; i++)
				{
					auto& al = als[i];

					switch (al.second ? al.second->width_policy : SizeFixed)
					{
					case SizeFixed:
						w = max(al.first->size.x(), w);
						break;
					case SizeFitParent:
						break;
					case SizeGreedy:
						w = max(al.second->min_size.x(), w);
						break;
					}
					switch (al.second ? al.second->height_policy : SizeFixed)
					{
					case SizeFixed:
						h += al.first->size.y();
						break;
					case SizeFitParent:
						factor += al.second->height_factor;
						break;
					case SizeGreedy:
						factor += al.second->height_factor;
						h += al.second->min_size.y();
						break;
					}
					h += item_padding;
				}
				if (fence > 0 && !als.empty())
					h -= item_padding;
				content_size = Vec2f(w, h);
				w += element->inner_padding_horizontal();
				h += element->inner_padding_vertical();

				if (width_fit_children)
				{
					if (aligner && aligner->width_policy == SizeGreedy)
					{
						aligner->min_size.x() = w;
						element->size.x() = max(element->size.x(), w);
					}
					else
						element->size.x() = w;
				}
				for (auto i = 0; i < n; i++)
					apply_h_free_layout(als[i], true);
				for (auto i = n; i < als.size(); i++)
					apply_h_free_layout(als[i], false);
				if (height_fit_children)
				{
					if (aligner && aligner->height_policy == SizeGreedy)
					{
						aligner->min_size.y() = h;
						element->size.y() = max(element->size.y(), h);
					}
					else
						element->size.y() = h;
				}
				h = element->size.y() - h;
				if (h > 0.f && factor > 0)
					h /= factor;
				else
					h = 0.f;
				auto y = element->inner_padding[1];
				for (auto i = 0; i < n; i++)
				{
					auto& al = als[i];

					if (al.second)
					{
						if (al.second->height_policy == SizeFitParent)
							al.first->size.y() = h * al.second->height_factor;
						else if (al.second->height_policy == SizeGreedy)
							al.first->size.y() = al.second->min_size.y() + h * al.second->height_factor;
					}
					assert(!al.second || al.second->y_align == AlignyFree);
					al.first->pos.y() = scroll_offset.y() + y;
					y += al.first->size.y() + item_padding;
				}
				for (auto i = n; i < als.size(); i++)
					apply_v_free_layout(als[i], false);
			}
				break;
			}
		}

		Component* copy()
		{
			auto copy = new cLayoutPrivate(type);

			copy->item_padding = item_padding;
			copy->width_fit_children = width_fit_children;
			copy->height_fit_children = height_fit_children;

			return copy;
		}
	};

	void cLayout::start()
	{
		((cLayoutPrivate*)this)->start();
	}

	void cLayout::update()
	{
		((cLayoutPrivate*)this)->update();
	}

	Component* cLayout::copy()
	{
		return ((cLayoutPrivate*)this)->copy();
	}

	cLayout* cLayout::create(LayoutType$ type)
	{
		return new cLayoutPrivate(type);
	}
}
