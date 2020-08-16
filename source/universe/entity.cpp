#include <flame/foundation/typeinfo.h>
#include <flame/universe/component.h>
#include "entity_private.h"
#include "world_private.h"

namespace flame
{
	EntityPrivate::EntityPrivate()
	{
		created_frame = looper().get_frame();

#ifdef _DEBUG
		//_created_stack_ = get_stack_frames(); TODO
#endif
	}

	EntityPrivate::~EntityPrivate()
	{
		for (auto& e : children)
		{
			for (auto c : e->local_message_dispatch_list)
				c->on_local_message(MessageRemoved);
		}
		for (auto c : local_message_dispatch_list)
			c->on_local_message(MessageDestroyed);
	}

	void EntityPrivate::release()
	{
		if (parent)
			parent->remove_child(this);
		else
			f_delete(this);
	}

	void EntityPrivate::update_visibility()
	{
		auto prev_visibility = global_visibility;
		if (parent)
			global_visibility = visible && parent->global_visibility;
		else
		{
			if (!world || world->root.get() == this)
				global_visibility = true;
			else
				global_visibility = false;
		}
		if (global_visibility != prev_visibility)
		{
			for (auto c : local_message_dispatch_list)
				c->on_local_message(MessageVisibilityChanged);
			if (parent)
			{
				for (auto c : parent->child_message_dispatch_list)
					c->on_local_message(MessageVisibilityChanged, this);
			}
		}

		for (auto& e : children)
			e->update_visibility();
	}

	void EntityPrivate::set_visible(bool v)
	{
		if (visible == v)
			return;
		visible = v;
		update_visibility();
	}

	void EntityPrivate::set_state(StateFlags s)
	{
		if (state == s)
			return;
		state = s;
		for (auto c : local_message_dispatch_list)
			c->on_local_message(MessageStateChanged);
	}

	void EntityPrivate::on_message(Message msg)
	{
		for (auto c : local_message_dispatch_list)
			c->on_local_message(msg);
	}

	Component* EntityPrivate::get_component(uint64 hash) const
	{
		auto it = components.find(hash);
		if (it != components.end())
			return it->second.get();
		return nullptr;
	}

	void EntityPrivate::Ref::gain(Component* c)
	{
		if (staging == INVALID_POINTER)
			return;
		target = staging;
		staging = INVALID_POINTER;
		*dst = target;
		if (target)
		{
			if (on_gain)
				on_gain(c);

			if (type == RefComponent)
				((ComponentAux*)((Component*)target)->aux)->list_ref_by.push_back(c);
		}
	}

	void EntityPrivate::Ref::lost(Component* c)
	{
		if (on_lost)
			on_lost(c);
		target = nullptr;
		*dst = nullptr;
	}

	void EntityPrivate::traversal(const std::function<bool(EntityPrivate*)>& callback)
	{
		if (!callback(this))
			return;
		for (auto& c : children)
			c->traversal(callback);
	}

	void EntityPrivate::add_component(Component* c)
	{
		assert(!c->entity);
		assert(components.find(c->type_hash) == components.end());

		ComponentAux aux;

		auto udt = find_udt(c->type_name);
		if (udt)
		{
			{
				auto u = find_udt((c->type_name + std::string("Private")).c_str());
				if (u)
					udt = u;
			}

			auto vc = udt->get_variables_count();
			for (auto i = 0; i < vc; i++)
			{
				auto v = udt->get_variable(i);
				auto m = v->get_meta();
				if (m->get_token("ref"))
				{
					Ref r;
					r.name = SUS::cut_tail_if(v->get_type()->get_name(), "Private");
					r.hash = std::hash<std::string>()(r.name);
					r.dst = (void**)((char*)c + v->get_offset());
					auto var_name = std::string(v->get_name());
					{
						auto f = udt->find_function(("on_gain_" + var_name).c_str());
						if (f)
							r.on_gain = a2f<void(*)(void*)>(f->get_address());
					}
					{
						auto f = udt->find_function(("on_lost_" + var_name).c_str());
						if (f)
							r.on_lost = a2f<void(*)(void*)>(f->get_address());
					}

					std::string place_str;
					m->get_token("place", &place_str);
					switch (r.name[(int)r.name.find_last_of(':') + 1])
					{
					case 'c':
					{
						r.type = RefComponent;

						if (place_str.empty() || place_str == "local")
						{
							r.place = PlaceLocal;
							r.staging = get_component(r.hash);
							if (!r.staging)
							{
								printf("add component %s failed, this component requires local component %s, which do not exist\n", c->type_name, r.name.c_str());
								return;
							}
						}
						else if (place_str == "parent")
						{
							r.place = PlaceParent;
							if (parent)
							{
								r.staging = parent->get_component(r.hash);
								if (!r.staging)
								{
									printf("add component %s failed, this component requires parent's component %s, which do not exist\n", c->type_name, r.name.c_str());
									return;
								}
							}
						}
						else if (place_str == "ancestor")
						{
							r.place = PlaceAncestor;
							if (parent)
							{
								r.staging = nullptr;
								auto e = parent;
								while (e)
								{
									r.staging = e->get_component(r.hash);
									if (r.staging)
										break;
									e = e->parent;
								}
								if (!r.staging)
								{
									printf("add component %s failed, this component requires ancestor's component %s, which do not exist\n", c->type_name, r.name.c_str());
									return;
								}
							}
						}
						else
						{
							r.place = PlaceOffspring;
							auto e = find_child(place_str);
							if (e)
								r.staging = e->get_component(r.hash);
							if (!r.staging)
							{
								printf("add component %s failed, this component requires offspring(%s)'s component %s, which do not exist\n", c->type_name, place_str.c_str(), r.name.c_str());
								return;
							}
						}

						aux.refs.push_back(r);
					}
						break;
					case 's':
						assert(place_str.empty());

						r.type = RefSystem;

						if (world)
						{
							r.staging = world->get_system(r.hash);
							if (!r.staging)
							{
								printf("add component %s failed, this component requires system %s, which do not exist\n", c->type_name, r.name.c_str());
								return;
							}
						}

						aux.refs.push_back(r);
						break;
					default:
						assert(place_str.empty());

						r.type = RefObject;

						if (world)
						{
							r.staging = world->find_object(r.name);
							if (!r.staging)
							{
								printf("add component %s failed, this component requires object %s, which do not exist\n", c->type_name, r.name.c_str());
								return;
							}
						}

						aux.refs.push_back(r);
					}
				}
			}
		}

		c->aux = new ComponentAux;
		*(ComponentAux*)c->aux = aux;

		c->entity = this;

		for (auto& r : ((ComponentAux*)c->aux)->refs)
			r.gain(c);

		if (udt->find_function("on_local_message"))
			aux.want_local_message = true;
		if (udt->find_function("on_child_message"))
			aux.want_child_message = true;
		if (udt->find_function("on_local_data_changed"))
			aux.want_local_data_changed = true;
		if (udt->find_function("on_child_data_changed"))
			aux.want_child_data_changed = true;

		c->on_added();

		for (auto cc : local_message_dispatch_list)
			cc->on_local_message(MessageComponentAdded, c);
		if (parent)
		{
			for (auto cc : parent->child_message_dispatch_list)
				cc->on_child_message(MessageComponentAdded, c);
		}

		components.emplace(c->type_hash, c);

		if (aux.want_local_message)
			local_message_dispatch_list.push_back(c);
		if (aux.want_child_message)
			child_message_dispatch_list.push_back(c);
		if (aux.want_local_data_changed)
			local_data_changed_dispatch_list.push_back(c);
		if (aux.want_child_data_changed)
			child_data_changed_dispatch_list.push_back(c);
	}

	void EntityPrivate::on_component_removed(Component* c)
	{
		auto& aux = *(ComponentAux*)c->aux;

		for (auto& r : aux.refs)
		{
			if (r.target)
			{
				if (r.type == RefComponent)
				{
					std::erase_if(((ComponentAux*)((Component*)r.target)->aux)->list_ref_by, [&](const auto& i) {
						return i == c;
					});
				}
				r.lost(c);
			}
		}

		delete &aux;
		c->aux = nullptr;

		for (auto cc : local_message_dispatch_list)
			cc->on_local_message(MessageComponentRemoved, c);
		if (parent)
		{
			for (auto cc : parent->child_message_dispatch_list)
				cc->on_child_message(MessageComponentRemoved, c);
		}
	}

	bool EntityPrivate::check_component_removable(Component* c) const
	{
		auto& aux = *(ComponentAux*)c->aux;

		if (!aux.list_ref_by.empty())
		{
			printf("remove component %s failed, this component is referenced by %d component(s)\n", c->type_name, aux.list_ref_by.size());
			return false;
		}

		return true;
	}

	void EntityPrivate::remove_component(Component* c, bool destroy)
	{
		auto it = components.find(c->type_hash);
		if (it == components.end())
		{
			assert(0);
			return;
		}

		if (!check_component_removable(c))
			return;

		auto& aux = *(ComponentAux*)c->aux;

		on_component_removed(c);

		if (aux.want_local_message)
		{
			std::erase_if(local_message_dispatch_list, [&](const auto& i) {
				return i == c;
			});
		}
		if (aux.want_child_message)
		{
			std::erase_if(child_message_dispatch_list, [&](const auto& i) {
				return i == c;
			});
		}
		if (aux.want_local_data_changed)
		{
			std::erase_if(local_data_changed_dispatch_list, [&](const auto& i) {
				return i == c;
			});
		}
		if (aux.want_child_data_changed)
		{
			std::erase_if(child_data_changed_dispatch_list, [&](const auto& i) {
				return i == c;
			});
		}

		if (!destroy)
			it->second.release();
		components.erase(it);
	}

	void EntityPrivate::remove_all_components(bool destroy)
	{
		for (auto& c : components)
		{
			if (!check_component_removable(c.second.get()))
				return;
		}

		for (auto& c : components)
			on_component_removed(c.second.get());

		local_message_dispatch_list.clear();
		child_message_dispatch_list.clear();
		local_data_changed_dispatch_list.clear();
		child_data_changed_dispatch_list.clear();

		if (!destroy)
		{
			for (auto& c : components)
				c.second.release();
		}
		components.clear();
	}

	void EntityPrivate::add_child(EntityPrivate* e, int position)
	{
		assert(e && e != this && !e->parent);

		auto ok = true;
		e->traversal([this, &ok](EntityPrivate* e) {
			for (auto& c : e->components)
			{
				auto& aux = *(ComponentAux*)c.second->aux;
				for (auto& r : aux.refs)
				{
					switch (r.type)
					{
					case RefComponent:
						if (!r.target)
						{
							switch (r.place)
							{
							case PlaceParent:
								r.staging = get_component(r.hash);
								if (!r.staging)
								{
									printf("add child failed, this child contains a component %s that requires parent's component %s, which do not exist\n", c.second->type_name, r.name.c_str());
									ok = false;
									return false;
								}
								break;
							case PlaceAncestor:
								r.staging = nullptr;
								auto e = this;
								while (e)
								{
									r.staging = e->get_component(r.hash);
									if (r.staging)
										break;
									e = e->parent;
								}
								break;
							}
						}
						break;
					case RefSystem:
						if (world)
						{
							r.staging = world->get_system(r.hash);
							if (!r.staging)
							{
								printf("add child failed, this child contains a component %s that requires system %s, which do not exist\n", c.second->type_name, r.name.c_str());
								ok = false;
								return false;
							}
						}
						break;
					case RefObject:
						if (world)
						{
							r.staging = world->find_object(r.name);
							if (!r.staging)
							{
								printf("add child failed, this child contains a component %s that requires object %s, which do not exist\n", c.second->type_name, r.name.c_str());
								ok = false;
								return false;
							}
						}
						break;
					}
				}
			}
			return true;
		});

		if (!ok)
			return;

		e->traversal([this](EntityPrivate* e) {
			for (auto& c : e->components)
			{
				auto& aux = *(ComponentAux*)c.second->aux;
				for (auto& r : aux.refs)
					r.gain(c.second.get());

				e->world = world;
				if (world)
					c.second->on_local_message(MessageEnteredWorld);
			}
			return true;
		});

		if (position == -1)
			position = children.size();

		children.emplace(children.begin() + position, e);

		e->parent = this;
		e->depth = depth + 1;
		e->index = position;
		e->update_visibility();

		for (auto c : e->local_message_dispatch_list)
			c->on_local_message(MessageAdded);
		for (auto c : child_message_dispatch_list)
			c->on_child_message(MessageAdded, e);
	}

	void EntityPrivate::reposition_child(uint pos1, uint pos2)
	{
		if (pos1 == pos2)
			return;
		assert(pos1 < children.size() && pos2 < children.size());

		auto a = children[pos1].get();
		auto b = children[pos2].get();
		a->index = pos2;
		b->index = pos1;
		std::swap(children[pos1], children[pos2]);

		for (auto c : a->local_message_dispatch_list)
			c->on_local_message(MessagePositionChanged);
		for (auto c : b->local_message_dispatch_list)
			c->on_local_message(MessagePositionChanged);

		for (auto c : child_message_dispatch_list)
			c->on_child_message(MessagePositionChanged, a);
		for (auto c : child_message_dispatch_list)
			c->on_child_message(MessagePositionChanged, b);
	}

	void EntityPrivate::on_child_removed(EntityPrivate* e) const
	{
		e->parent = nullptr;

		e->traversal([](EntityPrivate* e) {
			for (auto& c : e->components)
			{
				auto& aux = *(ComponentAux*)c.second->aux;
				for (auto& r : aux.refs)
				{
					if (r.target)
					{
						if (r.type == RefComponent)
						{
							if (r.place != PlaceLocal)
							{
								auto target = (Component*)r.target;
								std::erase_if(((ComponentAux*)target->aux)->list_ref_by, [&](const auto& i) {
									return i == c.second.get();
								});
								r.lost(c.second.get());
							}
						}
						else
							r.lost(c.second.get());
					}
				}
			}

			for (auto c : e->local_message_dispatch_list)
				c->on_local_message(MessageLeftWorld);
			e->world = nullptr;

			return true;
		});

		for (auto c : e->local_message_dispatch_list)
			c->on_local_message(MessageRemoved);
		for (auto c : child_message_dispatch_list)
			c->on_child_message(MessageRemoved, e);
	}

	static bool can_remove(EntityPrivate* e)
	{
		for (auto& c : e->components)
		{
			for (auto& r : ((EntityPrivate::ComponentAux*)c.second->aux)->list_ref_by)
			{
				if (((EntityPrivate*)r->entity)->depth < e->depth)
					return false;
			}
		}
		return true;
	}

	void EntityPrivate::remove_child(EntityPrivate* e, bool destroy)
	{
		assert(e && e != this);

		auto it = std::find_if(children.begin(), children.end(), [&](const auto& t) {
			return t.get() == e;
		});
		if (it == children.end())
		{
			assert(0); // not found!
			return;
		}

		if (!can_remove(e))
			return;

		on_child_removed(e);

		if (!destroy)
			it->release();
		children.erase(it);
	}

	void EntityPrivate::remove_all_children(bool destroy)
	{
		for (auto& c : children)
		{
			if (!can_remove(c.get()))
				return;
		}

		for (auto& c : children)
			on_child_removed(c.get());

		if (!destroy)
		{
			for (auto& c : children)
				c.release();
		}
		children.clear();
	}

	EntityPrivate* EntityPrivate::find_child(const std::string& name) const
	{
		for (auto& c : children)
		{
			if (c->name == name)
				return c.get();
			auto res = c->find_child(name);
			if (res)
				return res;
		}
		return nullptr;
	}

	void* EntityPrivate::add_data_changed_listener(void (*callback)(Capture& c, Component* t, uint64 data_name_hash), const Capture& capture)
	{
		auto c = new Closure(callback, capture);
		data_changed_listeners.emplace_back(c);
		return c;
	}

	void EntityPrivate::remove_data_changed_listener(void* lis)
	{
		std::erase_if(data_changed_listeners, [&](const auto& i) {
			return i == (decltype(i))lis;
		});
	}

	struct LoadState
	{
		std::vector<std::pair<std::string, std::string>> nss;
		std::filesystem::path path;

		std::string find_ns(const std::string& n)
		{
			for (auto& ns : nss)
			{
				if (ns.first == n)
					return ns.second;
			}
			return "";
		}
	};
	static std::stack<LoadState> load_states;

	static void load_prefab(EntityPrivate* dst, pugi::xml_node src)
	{
		if (src.name() != std::string("entity"))
			return;

		for (auto a : src.attributes())
		{
			auto name = std::string(a.name());
			if (name == "name")
				dst->name = a.value();
			else if (name == "visible")
				dst->visible = a.as_bool();
			else if (name == "src")
			{
				auto path = std::filesystem::path(a.value());
				path.replace_extension(L".prefab");
				dst->load(load_states.empty() ? path : load_states.top().path / path);
			}
		}

		for (auto n_c : src.children())
		{
			auto name = std::string(n_c.name());
			if (name == "entity")
			{
				auto e = f_new<EntityPrivate>();
				load_prefab(e, n_c);
				dst->add_child(e);
			}
			else if (name == "debug_break")
				debug_break();
			else
			{
				auto sp = SUS::split(name, ':');
				if (sp.size() == 2)
					name = load_states.top().find_ns(sp[0]) + "::" + sp[1];
				else
					name = "flame::" + name;
				auto udt = find_udt(name.c_str());
				if (udt)
				{
					auto c = dst->get_component(std::hash<std::string>()(name));
					if (!c)
					{
						auto fc = udt->find_function("create");
						if (fc->get_type()->get_tag() == TypePointer && fc->get_parameters_count() == 0)
						{
							fc->call(nullptr, &c, {});
							dst->add_component((Component*)c);
						}
					}
					if (c)
					{
						for (auto a : n_c.attributes())
						{
							auto fs = udt->find_function((std::string("set_") + a.name()).c_str());
							if (fs->get_type() == TypeInfo::get(TypeData, "void") && fs->get_parameters_count() == 1)
							{
								auto type = fs->get_parameter(0);
								void* d = type->create();
								type->unserialize(d, a.value());
								void* parms[] = { type->get_tag() == TypePointer ? *(void**)d : d };
								fs->call(c, nullptr, parms);
								type->destroy(d);
							}
							else
								printf("unknow attribute: %s\n", a.name());
						}
						for (auto n : n_c.children())
						{
							auto fs = udt->find_function((std::string("set_") + n.name()).c_str());
							if (fs->check(TypeInfo::get(TypeData, "void"), TypeInfo::get(TypePointer, "flame::Entity"), nullptr))
							{
								auto e = f_new<EntityPrivate>();
								load_prefab(e, n.child("entity"));
								a2f<void(*)(void*, void*)>(fs->get_address(c))(c, e);
							}
							else
								printf("unknow attribute: %s\n", n.name());
						}
					}
				}
				else
					printf("cannot find udt: %s\n", name.c_str());
			}
		}
	}

	void EntityPrivate::load(const std::filesystem::path& filename)
	{
		auto fn = filename;

		if (fn.parent_path().empty())
		{
			if (fn.extension().empty())
				fn.replace_extension(L".prefab");
			if (!std::filesystem::exists(fn))
			{
				fn = L"art" / fn;
				if (!std::filesystem::exists(fn))
				{
					auto engine_path = getenv("FLAME_PATH");
					if (engine_path)
						fn = engine_path / fn;
				}
			}
		}

		pugi::xml_document file;
		pugi::xml_node file_root;

		if (!file.load_file(fn.c_str()) || (file_root = file.first_child()).name() != std::string("prefab"))
		{
			printf("prefab not exist or wrong format: %s\n", filename.string().c_str());
			return;
		}

		LoadState state;
		for (auto a : file_root.attributes())
		{
			static std::regex reg_ns(R"(xmlns:(\w+))");
			std::smatch res;
			auto name = std::string(a.name());
			if (std::regex_search(name, res, reg_ns))
				state.nss.emplace_back(res[1].str(), a.value());
		}
		state.path = filename.parent_path();
		load_states.push(state);
		load_prefab(this, file_root.first_child());
		load_states.pop();
	}

	static void save_prefab(pugi::xml_node dst, EntityPrivate* src)
	{
		auto n = dst.append_child("entity");
		n.append_attribute("name").set_value(src->name.empty() ? "unnamed" : src->name.c_str());
		n.append_attribute("visible").set_value(src->visible);

		//if (!src->components.empty())
		//{
		//	auto n_cs = n.append_child("components");
		//	for (auto& c : src->components)
		//	{
		//		auto component = c.second.get();

		//		auto n_c = n_cs.append_child(component->name);

		//		auto udt = find_udt(component->name);
		//		assert(udt && udt->get_base_name() == std::string("Component"));
		//		auto variables_count = udt->get_variables_count();
		//		for (auto i = 0; i < variables_count; i++)
		//		{
		//			auto v = udt->get_variable(i);
		//			auto type = v->get_type();
		//			auto p = (char*)component + v->get_offset();
		//			if (type->get_tag() == TypePointer)
		//				//n_c.append_child(v->name.v).append_attribute("v").set_value((*(Object**)p)->id); TODO
		//				;
		//			else
		//			{
		//				auto dv = v->get_default_value();
		//				if (!dv || memcmp(dv, p, type->get_size()) != 0)
		//					n_c.append_child(v->get_name()).append_attribute("v").set_value(type->serialize(p).c_str());
		//			}
		//		}
		//	}
		//}

		//if (!src->children.empty())
		//{
		//	auto n_es = n.append_child("children");
		//	for (auto& e : src->children)
		//		save_prefab(n_es, e.get());
		//}
	}

	void EntityPrivate::save(const std::filesystem::path& filename)
	{
		pugi::xml_document file;
		auto file_root = file.append_child("prefab");

		save_prefab(file_root, this);

		file.save_file(filename.c_str());
	}

	void Entity::report_data_changed(Component* c, uint64 hash)
	{
		auto entity = (EntityPrivate*)c->entity;
		if (!entity)
			return;
		for (auto cc : entity->local_data_changed_dispatch_list)
		{
			if (cc != c)
				cc->on_local_data_changed(c, hash);
		}
		if (entity->parent)
		{
			for (auto cc : entity->parent->child_data_changed_dispatch_list)
				cc->on_child_data_changed(c, hash);
		}
		for (auto& l : entity->data_changed_listeners)
			l->call(c, hash);
	}

	Entity* Entity::create()
	{
		return f_new<EntityPrivate>();
	}
}
