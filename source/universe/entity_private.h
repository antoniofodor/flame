#pragma once

#include <flame/universe/entity.h>

#include <functional>

namespace flame
{
	struct UdtInfo;
	struct VariableInfo;

	struct WorldPrivate;

	struct EntityBridge : Entity
	{
		void add_child(Entity* e, int position) override;
		void remove_child(Entity* e, bool destroy) override;
		Entity* find_child(const char* name) const override;
		void load(const wchar_t* filename) override;
		void save(const wchar_t* filename) override;
	};

	struct EntityPrivate : EntityBridge
	{
		std::string name;

		bool visible = true;
		bool global_visibility = false;

		WorldPrivate* world = nullptr;
		EntityPrivate* parent = nullptr;

		StateFlags state = StateNone;

		uint depth = 0;
		uint index = 0;

		std::filesystem::path src;
		std::filesystem::path src_abs;

		int created_frame;
		int created_id;
		std::vector<void*> created_stack;
		std::filesystem::path created_filename;
		uint created_lineno = 0;

		enum RefType
		{
			RefComponent,
			RefSystem,
			RefObject
		};

		enum Place
		{
			PlaceLocal,
			PlaceParent,
			PlaceAncestor,
			PlaceOffspring
		};

		struct Ref
		{
			RefType type;
			Place place = PlaceLocal;
			std::string name;
			uint64 hash;
			bool optional = false;

			void* staging = INVALID_POINTER;
			void** dst = nullptr;
			void(*on_gain)(void*) = nullptr;
			void(*on_lost)(void*) = nullptr;

			void gain(Component* c);
			void lost(Component* c);
		};

		struct ComponentAux
		{
			std::vector<Ref> refs;

			std::vector<Component*> list_ref_by;

			bool want_local_message = false;
			bool want_child_message = false;
			bool want_local_data_changed = false;
			bool want_child_data_changed = false;
		};

		std::unordered_map<uint64, std::unique_ptr<Component, Delector>> components;
		std::vector<std::unique_ptr<EntityPrivate, Delector>> children;

		std::vector<Component*> local_message_dispatch_list;
		std::vector<Component*> child_message_dispatch_list;
		std::vector<Component*> local_data_changed_dispatch_list;
		std::vector<Component*> child_data_changed_dispatch_list;
		std::vector<std::unique_ptr<Closure<void(Capture&, Message, void*)>>> local_message_listeners;
		std::vector<std::unique_ptr<Closure<void(Capture&, Message, void*)>>> child_message_listeners;
		std::vector<std::unique_ptr<Closure<void(Capture&, Component*, uint64)>>> local_data_changed_listeners;

		std::vector<void*> events;

		std::vector<uint> local_data_changed_listeners_s;

		std::vector<std::pair<uint, void*>> events_s;

		EntityPrivate();
		~EntityPrivate();

		void release() override;

		const char* get_name() const override { return name.c_str(); };
		void set_name(const char* _name) override { name = _name; }

		bool get_visible() const override { return visible; }
		void update_visibility();
		void set_visible(bool v) override;

		World* get_world() const override { return (World*)world; }
		Entity* get_parent() const override { return parent; }

		StateFlags get_state() const override { return state; }
		void set_state(StateFlags state) override;

		void on_message(Message msg, void* p = nullptr) override;

		Component* get_component(uint64 hash) const override;
		Component* get_component_n(const char* name) const override;
		template <class T> inline T* get_parent_component_t() const { return !parent ? nullptr : parent->get_component_t<T>(); }
		void traversal(const std::function<bool(EntityPrivate*)>& callback);
		void add_component(Component* c);
		void on_component_removed(Component* c);
		bool check_component_removable(Component* c) const;
		void remove_component(Component* c, bool destroy = true);
		void remove_all_components(bool destroy) override;

		uint get_children_count() const override { return children.size(); }
		Entity* get_child(uint idx) const override { return children[idx].get(); }
		void add_child(EntityPrivate* e, int position = -1);
		void reposition_child(uint pos1, uint pos2) override;
		void on_child_removed(EntityPrivate* e) const;
		void remove_child(EntityPrivate* e, bool destroy = true);
		void remove_all_children(bool destroy) override;
		EntityPrivate* find_child(const std::string& name) const;

		void* add_local_message_listener(void (*callback)(Capture& c, Message msg, void* p), const Capture& capture) override;
		void remove_local_message_listener(void* lis) override;
		void* add_child_message_listener(void (*callback)(Capture& c, Message msg, void* p), const Capture& capture) override;
		void remove_child_message_listener(void* lis) override;
		void* add_local_data_changed_listener(void (*callback)(Capture& c, Component* t, uint64 hash), const Capture& capture) override;
		void remove_local_data_changed_listener(void* lis) override;

		void* add_event(void (*callback)(Capture& c), const Capture& capture) override;
		void remove_event(void* ev) override;

		void add_local_data_changed_listener_s(uint slot) override;
		void remove_local_data_changed_listener_s(uint slot) override;

		void add_event_s(uint slot) override;
		void remove_event_s(uint slot) override;

		void load(const std::filesystem::path& filename);
		void save(const std::filesystem::path& filename);

		const wchar_t* get_src() const override { return src.c_str(); }
	};

	inline void EntityBridge::add_child(Entity* e, int position)
	{
		((EntityPrivate*)this)->add_child((EntityPrivate*)e, position);
	}

	inline Entity* EntityBridge::find_child(const char* name) const
	{
		return ((EntityPrivate*)this)->find_child(name);
	}

	inline void EntityBridge::remove_child(Entity* e, bool destroy)
	{
		((EntityPrivate*)this)->remove_child((EntityPrivate*)e, destroy);
	}

	inline void EntityBridge::load(const wchar_t* filename)
	{
		((EntityPrivate*)this)->load(filename);
	}

	inline void EntityBridge::save(const wchar_t* filename)
	{
		((EntityPrivate*)this)->save(filename);
	}
}
