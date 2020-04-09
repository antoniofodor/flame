#include <flame/graphics/font.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/style.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/layout.h>
#include <flame/universe/components/menu.h>
#include <flame/universe/utils/layer.h>
#include <flame/universe/utils/style.h>
#include <flame/universe/utils/menu.h>

namespace flame
{
	struct cMenuPrivate : cMenu
	{
		void* mouse_listener;

		cMenuPrivate(Mode _mode)
		{
			element = nullptr;
			event_receiver = nullptr;

			root = nullptr;
			items = Entity::create();
			{
				auto ce = cElement::create();
				ce->frame_thickness = 1.f;
				ce->color = utils::style_4c(utils::FrameColorNormal).new_replacely<3>(255);
				ce->frame_color = utils::style_4c(utils::ForegroundColor);
				items->add_component(ce);
				items->add_component(cLayout::create(LayoutVertical));
				items->add_component(cMenuItems::create());
			}
			items->get_component(cMenuItems)->menu = this;
			mode = _mode;

			opened = false;

			mouse_listener = nullptr;
		}

		~cMenuPrivate()
		{
			if (!entity->dying_)
				event_receiver->mouse_listeners.remove(mouse_listener);
			Entity::destroy(items);
		}

		void close()
		{
			if (opened)
			{
				opened = false;

				for (auto i = 0; i < items->child_count(); i++)
				{
					auto menu = (cMenuPrivate*)items->child(i)->get_component(cMenu);
					if (menu)
						menu->close();
				}

				((Entity*)items->gene)->remove_child(items, false);
			}
		}

		void close_subs(Entity* p)
		{
			if (mode != ModeMain)
			{
				for (auto i = 0; i < p->child_count(); i++)
				{
					auto menu = (cMenuPrivate*)p->child(i)->get_component(cMenu);
					if (menu)
						menu->close();
				}
			}
		}

		void open(const Vec2f& pos)
		{
			if (!opened)
			{
				struct Capture
				{
					Entity* l;
					Entity* i;
				}capture;

				if (mode == ModeContext)
				{
					auto layer = utils::add_layer(root);
					layer->set_name("layer_menu");
					layer->on_removed_listeners.add([](void* c) {
						(*(Entity**)c)->remove_children(0, -1, false);
						return true;
					}, Mail::from_p(layer));
					auto items_element = items->get_component(cElement);
					items_element->set_pos(Vec2f(pos));
					items_element->set_scale(element->global_scale);
					capture.l = layer;
					capture.i = items;
					looper().add_event([](void* c, bool*) {
						auto& capture = *(Capture*)c;
						capture.l->add_child(capture.i);
					}, Mail::from_t(&capture));

					opened = true;
				}
				else
				{
					auto p = entity->parent();
					if (p)
						close_subs(p);

					auto layer = root->last_child();
					if (layer)
					{
						if (layer->name_hash() != FLAME_CHASH("layer_menu"))
							layer = nullptr;
					}
					auto new_layer = !layer;

					if (mode == ModeSub)
						assert(layer);
					else
					{
						if (mode == ModeMenubar)
						{
							if (!layer)
								layer = utils::add_layer(root, p);
						}
						else
							layer = utils::add_layer(root, entity);
					}
					if (new_layer)
					{
						layer->set_name("layer_menu");
						layer->on_removed_listeners.add([](void* c) {
							(*(Entity**)c)->remove_children(0, -1, false);
							return true;
						}, Mail::from_p(layer));
					}

					auto items_element = items->get_component(cElement);
					switch (mode)
					{
					case ModeMenubar: case ModeMain:
						items_element->set_pos(Vec2f(element->global_pos.x(), element->global_pos.y() + element->global_size.y()));
						break;
					case ModeSub:
						items_element->set_pos(Vec2f(element->global_pos.x() + element->global_size.x(), element->global_pos.y()));
						break;
					}
					items_element->set_scale(element->global_scale);
					capture.l = layer;
					capture.i = items;
					looper().add_event([](void* c, bool*) {
						auto& capture = *(Capture*)c;
						capture.l->add_child(capture.i);
					}, Mail::from_t(&capture));

					opened = true;
				}
			}
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
				element = (cElement*)c;
			else if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
					auto thiz = *(cMenuPrivate**)c;
					if (utils::is_menu_can_open(thiz, action, key))
						thiz->open((Vec2f)pos);
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cMenu* cMenu::create(cMenu::Mode mode)
	{
		return new cMenuPrivate(mode);
	}

	struct cMenuItemsPrivate : cMenuItems
	{
		void* on_removed_listener;

		cMenuItemsPrivate()
		{
			menu = nullptr;

			on_removed_listener = nullptr;
		}

		~cMenuItemsPrivate()
		{
			if (!entity->dying_)
				entity->on_removed_listeners.remove(on_removed_listener);
		}

		void on_added() override
		{
			if (!on_removed_listener)
			{
				on_removed_listener = entity->on_removed_listeners.add([](void* c) {
					auto thiz = *(cMenuItemsPrivate**)c;
					if (thiz->menu)
						thiz->menu->opened = false;
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cMenuItems* cMenuItems::create()
	{
		return new cMenuItemsPrivate();
	}

	struct cMenuItemPrivate : cMenuItem
	{
		void* mouse_listener;

		cMenuItemPrivate()
		{
			mouse_listener = nullptr;
		}

		~cMenuItemPrivate()
		{
			if (!entity->dying_)
				event_receiver->mouse_listeners.remove(mouse_listener);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cEventReceiver"))
			{
				event_receiver = (cEventReceiver*)c;
				mouse_listener = event_receiver->mouse_listeners.add([](void* c, KeyStateFlags action, MouseKey key, const Vec2i& pos) {
					auto thiz = *(cMenuItemPrivate**)c;
					auto c_items = thiz->entity->parent()->get_component(cMenuItems);
					if (c_items)
					{
						auto menu = (cMenuPrivate*)c_items->menu;
						auto layer = menu->root->last_child();
						if (layer)
						{
							if (layer->name_hash() == FLAME_CHASH("layer_menu"))
								menu->close_subs(menu->items);
						}
					}
					return true;
				}, Mail::from_p(this));
			}
		}
	};

	cMenuItem* cMenuItem::create()
	{
		return new cMenuItemPrivate();
	}
}
