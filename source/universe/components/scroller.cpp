#include "../entity_private.h"
#include "element_private.h"
#include "event_receiver_private.h"
#include "layout_private.h"
#include "scroller_private.h"
#include "../systems/event_dispatcher_private.h"

namespace flame
{
	void cScrollerPrivate::scroll(const Vec2f& v)
	{
		if (view_layout && target_element)
		{
			if (htrack_element)
			{
				auto target_size = target_element->width;
				if (target_size > view_element->width)
					hthumb_element->set_width(view_element->width / target_size * htrack_element->width);
				else
					hthumb_element->set_width(0.f);
				auto x = v.x() + hthumb_element->x;
				hthumb_element->set_x(hthumb_element->width > 0.f ? clamp(x, 0.f, htrack_element->width - hthumb_element->width) : 0.f);
				view_layout->set_scrollx(-int(hthumb_element->x / htrack_element->width * target_size / step) * step);
			}
			if (vtrack_element)
			{
				auto target_size = target_element->height;
				if (target_size > view_element->height)
					vthumb_element->set_height(view_element->height / target_size * vtrack_element->height);
				else
					vthumb_element->set_height(0.f);
				auto y = v.y() + vthumb_element->y;
				vthumb_element->set_y(vthumb_element->height > 0.f ? clamp(y, 0.f, vtrack_element->height - vthumb_element->height) : 0.f);
				view_layout->set_scrolly(-int(vthumb_element->y / vtrack_element->height * target_size / step) * step);
			}
		}
	}

	void cScrollerPrivate::on_gain_event_receiver()
	{
		mouse_scroll_listener = event_receiver->add_mouse_scroll_listener([](Capture& c, int v) {
			c.thiz<cScrollerPrivate>()->scroll(Vec2f(0.f, -v * 20.f));
		}, Capture().set_thiz(this));
	}

	void cScrollerPrivate::on_lost_event_receiver()
	{
		event_receiver->remove_mouse_scroll_listener(mouse_scroll_listener);
	}

	void cScrollerPrivate::on_gain_htrack_element()
	{
		htrack_element_listener = htrack_element->entity->add_local_data_changed_listener([](Capture& c, Component* t, uint64 h) {
			auto thiz = c.thiz<cScrollerPrivate>();
			if (t == thiz->htrack_element)
			{
				if (h == S<ch("width")>::v)
					thiz->scroll(Vec2f(0.f));
			}
		}, Capture().set_thiz(this));
	}

	void cScrollerPrivate::on_lost_htrack_element()
	{
		htrack_element->entity->remove_local_data_changed_listener(htrack_element_listener);
	}

	void cScrollerPrivate::on_gain_hthumb_event_receiver()
	{
		hthumb_mouse_listener = hthumb_event_receiver->add_mouse_move_listener([](Capture& c, const Vec2i& disp, const Vec2i& pos) {
			auto thiz = c.thiz<cScrollerPrivate>();
			if (thiz->event_receiver->dispatcher->active == thiz->hthumb_event_receiver)
				thiz->scroll(Vec2f(disp.x(), 0.f));
		}, Capture().set_thiz(this));
	}

	void cScrollerPrivate::on_lost_hthumb_event_receiver()
	{
		hthumb_event_receiver->remove_mouse_move_listener(hthumb_mouse_listener);
	}

	void cScrollerPrivate::on_gain_vtrack_element()
	{
		vtrack_element_listener = vtrack_element->entity->add_local_data_changed_listener([](Capture& c, Component* t, uint64 h) {
			auto thiz = c.thiz<cScrollerPrivate>();
			if (t == thiz->vtrack_element)
			{
				if (h == S<ch("height")>::v)
					thiz->scroll(Vec2f(0.f));
			}
		}, Capture().set_thiz(this));
	}

	void cScrollerPrivate::on_lost_vtrack_element()
	{
		vtrack_element->entity->remove_local_data_changed_listener(vtrack_element_listener);
	}

	void cScrollerPrivate::on_gain_vthumb_event_receiver()
	{
		vthumb_mouse_listener = vthumb_event_receiver->add_mouse_move_listener([](Capture& c, const Vec2i& disp, const Vec2i& pos) {
			auto thiz = c.thiz<cScrollerPrivate>();
			if (thiz->event_receiver->dispatcher->active == thiz->vthumb_event_receiver)
				thiz->scroll(Vec2f(0.f, disp.y()));
		}, Capture().set_thiz(this));
	}

	void cScrollerPrivate::on_lost_vthumb_event_receiver()
	{
		vthumb_event_receiver->remove_mouse_move_listener(vthumb_mouse_listener);
	}

	void cScrollerPrivate::on_gain_view_element()
	{
		((EntityPrivate*)view_element->entity)->add_child_message_listener([](Capture& c, Message msg, void* p) {
			auto thiz = c.thiz<cScrollerPrivate>();
			switch (msg)
			{
			case MessageAdded:
				if (!thiz->target_element)
				{
					auto e = (EntityPrivate*)p;
					auto ce = e->get_component_t<cElementPrivate>();
					if (ce)
					{
						thiz->target_element = ce;

						thiz->scroll(Vec2f(0.f));

						e->add_local_data_changed_listener([](Capture& c, Component* t, uint64 h) {
							auto thiz = c.thiz<cScrollerPrivate>();
							if (t == thiz->target_element)
							{
								if (h == S<ch("width")>::v || h == S<ch("height")>::v)
									thiz->scroll(Vec2f(0.f));
							}
						}, Capture().set_thiz(thiz));
					}
				}
				break;
			case MessageRemoved:
				if (thiz->target_element && thiz->target_element == ((EntityPrivate*)p)->get_component_t<cElementPrivate>())
					thiz->target_element = nullptr;
				break;
			}
		}, Capture().set_thiz(this));

		((EntityPrivate*)view_element->entity)->add_local_data_changed_listener([](Capture& c, Component* t, uint64 h) {
			auto thiz = c.thiz<cScrollerPrivate>();
			if (t == thiz->view_element)
			{
				if (h == S<ch("width")>::v || h == S<ch("height")>::v)
					thiz->scroll(Vec2f(0.f));
			}
		}, Capture().set_thiz(this));
	}

	bool cScrollerPrivate::check_refs()
	{
		return (htrack_element && hthumb_element && hthumb_event_receiver) ||
			(vtrack_element && vthumb_element && vthumb_event_receiver);
	}

	cScroller* cScroller::create()
	{
		return f_new<cScrollerPrivate>();
	}
}
