#include "../../graphics/model.h"
#include "../entity_private.h"
#include "../world_private.h"
#include "node_private.h"
#include "armature_private.h"
#include "../systems/renderer_private.h"

namespace flame
{
	mat4 cArmaturePrivate::Bone::calc_mat()
	{
		return node->transform * offmat;
	}

	void cArmaturePrivate::Animation::apply(Bone* bones, uint playing_frame)
	{
		for (auto& t : tracks)
		{
			auto& b = bones[t.first];
			auto& k = t.second[playing_frame];
			b.node->set_pos(k.p);
			b.node->set_qut(k.q);
		}
	}

	cArmaturePrivate::~cArmaturePrivate()
	{
		node->drawers.remove("armature"_h);
		node->measurers.remove("armature"_h);
	}

	void cArmaturePrivate::on_init()
	{
		node->drawers.add([this](sRendererPtr renderer) {
			draw(renderer);
		}, "armature"_h);

		node->measurers.add([this](AABB* ret) {
			if (!model)
				return false;
			*ret = AABB(model->bounds.get_points(node->transform));
			return true;
		}, "armature"_h);

		node->mark_transform_dirty();
	}

	void cArmaturePrivate::set_model_name(const std::filesystem::path& path)
	{
		if (model_name == path)
			return;
		bones.clear();
		model_name = path;
		apply_src();
		if (node)
			node->mark_transform_dirty();
		data_changed("model_name"_h);
	}

	void cArmaturePrivate::set_animation_names(const std::wstring& paths)
	{
		if (animation_names == paths)
			return;
		animation_names = paths;
		apply_src();
		if (node)
			node->mark_transform_dirty();
		data_changed("animation_names"_h);
	}

	void cArmaturePrivate::play(uint id)
	{
		if (playing_id == id)
			return;
		stop();
		playing_id = id;
	}

	void cArmaturePrivate::stop()
	{
		playing_id = -1;
		playing_frame = 0;
		time = 0.f;
		playing_id = -1;
	}

	void cArmaturePrivate::apply_src()
	{
		model = graphics::Model::get(model_name);
		if (!model)
			return;

		bones.resize(model->bones.size());
		for (auto i = 0; i < bones.size(); i++)
		{
			auto& src = model->bones[i];
			auto& dst = bones[i];
			auto name = src.name;
			auto e = entity->find_child(name);
			if (e)
			{
				dst.name = name;
				dst.node = e->get_component_i<cNodeT>(0);
				if (dst.node)
					dst.offmat = src.offset_matrix;
			}
		}

		if (bones.empty() || animation_names.empty())
			return;

		auto sp = SUW::split(animation_names, ';');
		for (auto& s : sp)
		{
			auto animation = graphics::Animation::get(s);
			if (animation)
			{
				auto& a = animations.emplace_back();
				a.total_frame = 0;

				for (auto& ch : animation->channels)
				{
					auto find_bone = [&](std::string_view name) {
						for (auto i = 0; i < bones.size(); i++)
						{
							if (bones[i].name == name)
								return i;
						}
						return -1;
					};
					auto id = find_bone(ch.node_name);
					if (id != -1)
					{
						uint count = ch.keys.size();
						if (a.total_frame == 0)
							a.total_frame = max(a.total_frame, count);

						auto& t = a.tracks.emplace_back();
						t.first = id;
						t.second.resize(count);
						memcpy(t.second.data(), ch.keys.data(), sizeof(graphics::Channel::Key) * count);
					}
				}

				for (auto& t : a.tracks)
					t.second.resize(a.total_frame);
			}
		}
	}

	void cArmaturePrivate::draw(sRendererPtr renderer)
	{
		if (instance_id == -1)
			return;

		if (frame < (int)frames)
		{
			if (playing_id != -1)
			{
				auto& a = animations[playing_id];
				a.apply(bones.data(), playing_frame);

				time += playing_speed;
				while (time > 1.f)
				{
					time -= 1.f;

					playing_frame++;
					if (playing_frame == a.total_frame)
						playing_frame = loop ? 0 : -1;

					if (playing_frame == -1)
					{
						stop();
						break;
					}
				}
			}

			auto dst = renderer->set_armature_instance(instance_id);
			for (auto i = 0; i < bones.size(); i++)
				dst[i] = bones[i].calc_mat();

			frame = frames;
		}
	}

	void cArmaturePrivate::on_active()
	{
		apply_src();

		instance_id = sRenderer::instance()->register_armature_instance(-1);

		node->mark_transform_dirty();
	}

	void cArmaturePrivate::on_inactive()
	{
		model = nullptr;

		stop();
		bones.clear();
		animations.clear();

		sRenderer::instance()->register_armature_instance(instance_id);
		instance_id = -1;
	}

	struct cArmatureCreate : cArmature::Create
	{
		cArmaturePtr operator()(EntityPtr e) override
		{
			return new cArmaturePrivate();
		}
	}cArmature_create;
	cArmature::Create& cArmature::create = cArmature_create;
}
