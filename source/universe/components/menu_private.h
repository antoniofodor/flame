#include <flame/universe/components/menu.h>

namespace flame
{
	struct EntityPrivate;
	struct cElementPrivate;
	struct cEventReceiverPrivate;

	struct cMenuPrivate : cMenu // R ~ on_*
	{
		cElementPrivate* element = nullptr; // R ref
		cEventReceiverPrivate* event_receiver = nullptr; // R ref

		void* mouse_down_listener = nullptr;
		void* mouse_move_listener = nullptr;
		void* root_mouse_listener = nullptr;
		
		MenuType type = MenuTop;
		EntityPrivate* items = nullptr;
		EntityPrivate* root = nullptr;
		cEventReceiverPrivate* root_event_receiver = nullptr;
		bool opened = false;
		int frame = -1;

		MenuType get_type() const override { return type; }
		void set_type(MenuType t) override { type = t; }

		void open();
		void close();

		void on_gain_event_receiver();
		void on_lost_event_receiver();

		void on_local_message(Message msg, void* p) override;
		void on_child_message(Message msg, void* p) override;
	};
}
