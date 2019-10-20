#include <flame/universe/components/element.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/splitter.h>

namespace flame
{
	struct cSplitterPrivate : cSplitter
	{
		void* mouse_listener;

		cSplitterPrivate()
		{
			event_receiver = nullptr;

			type = SplitterHorizontal;

			mouse_listener = nullptr;
		}

		~cSplitterPrivate()
		{
			if (!entity->dying_)
				event_receiver->mouse_listeners.remove(mouse_listener);
		}

		void on_component_added(Component* c) override
		{
			if (c->type_hash == cH("EventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
					auto thiz = (*(cSplitterPrivate**)c);
					if (thiz->event_receiver->active && is_mouse_move(action, key))
					{
						auto parent = thiz->entity->parent();
						auto idx = parent->child_position(thiz->entity);
						if (idx > 0 && idx < parent->child_count() - 1)
						{
							auto left = parent->child(idx - 1);
							auto left_element = left->get_component(Element);
							assert(left_element);
							auto left_aligner = left->get_component(Aligner);
							auto right = parent->child(idx + 1);
							auto right_element = right->get_component(Element);
							assert(right_element);
							auto right_aligner = right->get_component(Aligner);

							if (thiz->type == SplitterHorizontal)
							{
								if (pos.x() < 0.f)
								{
									auto v = min(left_element->size.x() - max(1.f, left_aligner ? left_aligner->min_size.x() : left_element->inner_padding_horizontal()), (float)-pos.x());
									left_element->size.x() -= v;
									right_element->size.x() += v;
								}
								else if (pos.x() > 0.f)
								{
									auto v = min(right_element->size.x() - max(1.f, right_aligner ? right_aligner->min_size.x() : right_element->inner_padding_horizontal()), (float)pos.x());
									left_element->size.x() += v;
									right_element->size.x() -= v;
								}
								if (left_aligner)
									left_aligner->width_factor = left_element->size.x();
								if (right_aligner)
									right_aligner->width_factor = right_element->size.x();
							}
							else
							{
								if (pos.y() < 0.f)
								{
									auto v = min(left_element->size.y() - max(1.f, left_aligner ? left_aligner->min_size.y() : left_element->inner_padding_vertical()), (float)-pos.y());
									left_element->size.y() -= v;
									right_element->size.y() += v;
								}
								else if (pos.y() > 0.f)
								{
									auto v = min(right_element->size.y() - max(1.f, right_aligner ? right_aligner->min_size.y() : right_element->inner_padding_vertical()), (float)pos.y());
									left_element->size.y() += v;
									right_element->size.y() -= v;
								}
								if (left_aligner)
									left_aligner->height_factor = left_element->size.y();
								if (right_aligner)
									right_aligner->height_factor = right_element->size.y();
							}
						}
					}
				}, new_mail_p(this));
			}
		}

		Component* copy() override
		{
			auto copy = new cSplitterPrivate();

			copy->type = type;

			return copy;
		}
	};

	cSplitter* cSplitter::create()
	{
		return new cSplitterPrivate;
	}
}
