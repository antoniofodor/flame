#include <flame/foundation/serialize.h>
#include <flame/universe/entity.h>
#include <flame/universe/component.h>

namespace flame
{
	struct EntityPrivate : Entity
	{
		std::string name;
		uint name_hash;
		std::vector<std::unique_ptr<Component>> components;
		EntityPrivate* parent;
		std::vector<std::unique_ptr<EntityPrivate>> children;

		EntityPrivate() :
			parent(nullptr)
		{
			created_frame = looper().frame;

			visible = true;
			global_visible = false;

			first_update = true;

			name_hash = 0;
		}

		Component* find_component(uint type_hash)
		{
			for (auto& c : components)
			{
				if (c->type_hash == type_hash)
					return c.get();
			}
			return nullptr;
		}

		Mail<std::vector<Component*>> find_components(uint type_hash)
		{
			auto ret = new_mail<std::vector<Component*>>();
			for (auto& c : components)
			{
				if (c->type_hash == type_hash)
					ret.p->push_back(c.get());
			}
			return ret;
		}

		void add_component(Component* c)
		{
			c->entity = this;
			components.emplace_back(c);
		}

		EntityPrivate* find_child(const std::string& name) const
		{
			for (auto& e : children)
			{
				if (e->name == name)
					return e.get();
			}
			return nullptr;
		}

		void add_child(EntityPrivate* e, int position)
		{
			if (position == -1)
				position = children.size();
			children.insert(children.begin() + position, std::unique_ptr<EntityPrivate>(e));
			e->parent = this;
			for (auto& c : e->components)
				c->on_entity_added_to_parent();
		}

		void reposition_child(EntityPrivate* e, int position)
		{
			if (position == -1)
				position = children.size() - 1;
			assert(position < children.size());
			if (children[position].get() == e)
				return;
			for (auto& _e : children)
			{
				if (_e.get() == e)
				{
					std::swap(_e, children[position]);
					break;
				}
			}
		}

		void remove_child(EntityPrivate* e)
		{
			for (auto it = children.begin(); it != children.end(); it++)
			{
				if (it->get() == e)
				{
					children.erase(it);
					return;
				}
			}
		}

		void take_child(EntityPrivate* e)
		{
			for (auto it = children.begin(); it != children.end(); it++)
			{
				if (it->get() == e)
				{
					e->parent = nullptr;
					it->release();
					children.erase(it);
					return;
				}
			}
		}

		void remove_all_children()
		{
			children.clear();
		}

		void take_all_children()
		{
			for (auto& e : children)
			{
				e->parent = nullptr;
				e.release();
			}
			children.clear();
		}

		EntityPrivate* copy()
		{
			auto e = new EntityPrivate;

			e->visible = visible;
			e->set_name(name);
			for (auto& c : components)
			{
				auto copy = c->copy();
				e->add_component(copy);
			}
			for (auto& e : children)
			{
				auto copy = e->copy();
				e->add_child(copy, -1);
			}

			return e;
		}

		void traverse_forward(void (*callback)(void* c, Entity* n), const Mail<>& capture)
		{
			callback(capture.p, this);
			for (auto& c : children)
			{
				if (c->global_visible)
					c->traverse_forward(callback, capture);
			}
		}

		void traverse_backward(void (*callback)(void* c, Entity* n), const Mail<>& capture)
		{
			for (auto it = children.rbegin(); it != children.rend(); it++)
			{
				auto c = it->get();
				if (c->global_visible)
					c->traverse_backward(callback, capture);
			}
			callback(capture.p, this);
		}

		void update()
		{
			if (!parent)
				global_visible = visible;
			else
				global_visible = visible && parent->global_visible;
			if (!global_visible)
				return;
			if (first_update)
			{
				first_update = false;
				for (auto& c : components)
					c->start();
			}
			for (auto& c : components)
				c->update();
			for (auto& e : children)
				e->update();
		}
	};

	const std::string& Entity::name() const
	{
		return ((EntityPrivate*)this)->name;
	}

	uint Entity::name_hash() const
	{
		return ((EntityPrivate*)this)->name_hash;
	}

	void Entity::set_name(const std::string& name) const
	{
		auto thiz = ((EntityPrivate*)this);
		thiz->name = name;
		thiz->name_hash = H(name.c_str());
	}

	uint Entity::component_count() const
	{
		return ((EntityPrivate*)this)->components.size();
	}

	Component* Entity::component(uint index) const
	{
		return ((EntityPrivate*)this)->components[index].get();
	}

	Component* Entity::find_component(uint type_hash) const
	{
		return ((EntityPrivate*)this)->find_component(type_hash);
	}

	Mail<std::vector<Component*>> Entity::find_components(uint type_hash) const
	{
		return ((EntityPrivate*)this)->find_components(type_hash);
	}

	void Entity::add_component(Component* c)
	{
		((EntityPrivate*)this)->add_component(c);
	}

	Entity* Entity::parent() const
	{
		return ((EntityPrivate*)this)->parent;
	}

	uint Entity::child_count() const
	{
		return ((EntityPrivate*)this)->children.size();
	}

	int Entity::child_position(Entity* e) const
	{
		auto& children = ((EntityPrivate*)this)->children;
		for (auto i = 0; i < children.size(); i++)
		{
			if (children[i].get() == e)
				return i;
		}
		return -1;
	}

	Entity* Entity::child(uint index) const
	{
		return ((EntityPrivate*)this)->children[index].get();
	}

	Entity* Entity::find_child(const std::string& name) const
	{
		return ((EntityPrivate*)this)->find_child(name);
	}

	void Entity::add_child(Entity* e, int position)
	{
		((EntityPrivate*)this)->add_child((EntityPrivate*)e, position);
	}

	void Entity::reposition_child(Entity* e, int position)
	{
		((EntityPrivate*)this)->reposition_child((EntityPrivate*)e, position);
	}

	void Entity::remove_child(Entity* e)
	{
		((EntityPrivate*)this)->remove_child((EntityPrivate*)e);
	}

	void Entity::take_child(Entity* e)
	{
		((EntityPrivate*)this)->take_child((EntityPrivate*)e);
	}

	void Entity::remove_all_children()
	{
		((EntityPrivate*)this)->remove_all_children();
	}

	void Entity::take_all_children()
	{
		((EntityPrivate*)this)->take_all_children();
	}

	Entity* Entity::copy()
	{
		return ((EntityPrivate*)this)->copy();
	}

	void Entity::traverse_forward(void (*callback)(void* c, Entity* n), const Mail<>& capture)
	{
		((EntityPrivate*)this)->traverse_forward(callback, capture);
		delete_mail(capture);
	}

	void Entity::traverse_backward(void (*callback)(void* c, Entity* n), const Mail<>& capture)
	{
		((EntityPrivate*)this)->traverse_backward(callback, capture);
		delete_mail(capture);
	}

	void Entity::update()
	{
		((EntityPrivate*)this)->update();
	}

	//static void serialize(EntityPrivate* src, SerializableNode* dst)
	//{
	//	dst->new_attr("name", src->name);
	//	dst->new_attr("visible", std::to_string((int)src->visible));
	//	auto n_components = dst->new_node("components");
	//	for (auto& c : src->components)
	//	{
	//		std::string type_name = c->type_name;
	//		auto udt = find_udt(H((type_name + "A").c_str()));
	//		if (udt)
	//		{
	//			auto create_func = udt->find_function("create");
	//			if (create_func)
	//				int cut = 1;
	//		}
	//	}
	//}

	//static void unserialize(EntityPrivate* dst, SerializableNode* src)
	//{

	//}

	Entity* Entity::create()
	{
		return new EntityPrivate;
	}

	Entity* Entity::create_from_file(const std::wstring& filename)
	{
		auto file = SerializableNode::create_from_xml_file(filename);
		if (!file || file->name() != "node")
			return nullptr;
		return nullptr;
	}

	void Entity::save_to_file(Entity* e, const std::wstring& filename)
	{

	}

	void Entity::destroy(Entity* w)
	{
		delete (EntityPrivate*)w;
	}

	void* component_alloc(uint size)
	{
		return malloc(size);
	}
}
