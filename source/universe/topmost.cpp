#include <flame/universe/topmost.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/menu.h>

namespace flame
{
	static Entity* topmost;

	Entity* get_topmost()
	{
		return topmost;
	}

	Entity* create_topmost(Entity* e, bool penetrable, bool close_when_clicked)
	{
		topmost = Entity::create();
		topmost->set_name("topmost");
		e->add_child(topmost);

		{
			auto e_c_element = (cElement*)e->find_component(cH("Element"));
			assert(e_c_element);

			auto c_element = cElement::create();
			c_element->width = e_c_element->width;
			c_element->height = e_c_element->height;
			topmost->add_component(c_element);

			auto c_event_receiver = cEventReceiver::create();
			c_event_receiver->penetrable = penetrable;
			c_event_receiver->add_mouse_listener([](void* c, KeyState action, MouseKey key, const Vec2f& pos) {
				if (is_mouse_down(action, key, true) && key == Mouse_Left && topmost->created_frame != looper().frame)
					destroy_topmost();
			}, Mail<>());
			topmost->add_component(c_event_receiver);
		}

		return topmost;
	}

	void destroy_topmost()
	{
		std::vector<Entity*> children;
		for (auto i = 0; i < topmost->child_count(); i++)
			children.push_back(topmost->child(i));
		for (auto e : children)
		{
			auto menu = (cMenu*)e->find_component(cH("Menu"));
			if (menu)
			{
				if (menu->popuped_by)
					menu->popuped_by->close();
				else
					close_menu(e);
			}
		}

		topmost->take_all_children();
		looper().add_delay_event([](void* c) {
			auto e = *(Entity**)c;
			e->parent()->take_child(e);
		}, new_mail_p(topmost));
		topmost = nullptr;
	}
}
