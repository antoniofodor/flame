#include "app.h"
#include "selection.h"
#include "view_inspector.h"
#include "dialog_procedure_terrain.h"

#include <flame/foundation/typeinfo.h>
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/foundation/bitmap.h>
#include <flame/graphics/extension.h>
#include <flame/graphics/material.h>
#include <flame/graphics/model.h>
#include <flame/graphics/animation.h>
#include <flame/graphics/shader.h>
#include <flame/graphics/debug.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/terrain.h>

View_Inspector view_inspector;
static auto selection_changed = false;

View_Inspector::View_Inspector() :
	GuiView("Inspector")
{
	selection.callbacks.add([](uint caller) {
		if (caller != "inspector"_h)
			selection_changed = true;
	}, "inspector"_h);
}

struct StagingVector
{
	std::vector<char> v;
	TypeInfo* type = nullptr;
	uint item_size = 0;

	StagingVector(TypeInfo* _type)
	{
		type = _type;
		item_size = type->size;
	}

	~StagingVector()
	{
		if (type)
		{
			auto n = count();
			for (auto i = 0; i < n; i++)
				type->destroy(v.data() + i * item_size, false);
			v.clear();
			type = nullptr;
			item_size = 0;
		}
	}

	uint count()
	{
		return v.size() / item_size;
	}

	void resize(void* _vec, uint size)
	{
		if (!_vec)
			_vec = &v;
		auto& vec = *(std::vector<char>*)_vec;
		auto old_size = vec.size() / item_size;
		v.resize(size * item_size);
		if (old_size < size)
		{
			for (auto i = old_size; i < size; i++)
				type->create(vec.data() + i * item_size);
		}
		else if (old_size > size)
		{
			for (auto i = size; i < old_size; i++)
				type->destroy(vec.data() + i * item_size, false);
		}
	}

	void assign(void* _dst, void* _src)
	{
		if (!_dst)
			_dst = &v;
		if (!_src)
			_src = &v;
		auto& dst = *(std::vector<char>*)_dst;
		auto& src = *(std::vector<char>*)_src;
		dst.resize(src.size());
		auto count = src.size() / item_size;
		for (auto i = 0; i < count; i++)
		{
			auto p = dst.data() + i * item_size;
			type->create(p);
			type->copy(p, src.data() + i * item_size);
		}
	}
};

std::map<const void*, StagingVector> staging_vectors;

StagingVector& get_staging_vector(const void* id, TypeInfo* type, void* vec)
{
	auto it = staging_vectors.find(id);
	if (it != staging_vectors.end())
		return it->second;
	auto& ret = staging_vectors.emplace(id, type).first->second;
	ret.assign(nullptr, vec);
	return ret;
}

std::string show_udt(const UdtInfo& ui, void* src);

bool show_variable(const UdtInfo& ui, TypeInfo* type, const std::string& name, int offset, int getter_idx, int setter_idx, void* src, const void* id) 
{
	auto changed = false;
	auto direct_io = getter_idx == -1 && setter_idx == -1;

	ImGui::PushID(id);
	switch (type->tag)
	{
	case TagD:
	{
		auto data = ui.get_value(type, src, offset, getter_idx, !direct_io);
		auto ti = (TypeInfo_Data*)type;
		switch (ti->data_type)
		{
		case DataBool:
			changed = ImGui::Checkbox(name.c_str(), (bool*)data);
			break;
		case DataInt:
			switch (ti->vec_size)
			{
			case 1:
				changed = ImGui::DragInt(name.c_str(), (int*)data);
				break;
			}
			break;
		case DataFloat:
			switch (ti->vec_size)
			{
			case 1:
				changed = ImGui::DragFloat(name.c_str(), (float*)data, 0.01f);
				break;
			case 2:
				changed = ImGui::DragFloat2(name.c_str(), (float*)data, 0.01f);
				break;
			case 3:
				changed = ImGui::DragFloat3(name.c_str(), (float*)data, 0.01f);
				break;
			case 4:
				changed = ImGui::DragFloat4(name.c_str(), (float*)data, 0.01f);
				break;
			}
			break;
		case DataString:
			changed = ImGui::InputText(name.c_str(), (std::string*)data);
			break;
		case DataWString:
			break;
		case DataPath:
		{
			auto& path = *(std::filesystem::path*)data;
			auto s = path.string();
			ImGui::InputText(name.c_str(), s.data(), ImGuiInputTextFlags_ReadOnly);
			if (ImGui::BeginDragDropTarget())
			{
				if (auto payload = ImGui::AcceptDragDropPayload("File"); payload)
				{
					path = Path::reverse(std::wstring((wchar_t*)payload->Data));
					changed = true;
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::SameLine();
			if (ImGui::Button("P"))
			{
				add_event([path]() {
					selection.select(Path::get(path), "project"_h);
					return false;
				});
			}
		}
			break;
		}
		if (changed && !direct_io)
			ui.set_value(type, src, offset, setter_idx, nullptr);
	}
		break;
	case TagVD:
		if (ImGui::TreeNode(name.c_str()))
		{
			assert(getter_idx == -1);
			auto pv = (char*)src + offset;
			auto ti = ((TypeInfo_VectorOfData*)type)->ti;
			auto& sv = get_staging_vector(id, ti, pv);
			if (ImGui::Button("Get"))
				sv.assign(nullptr, pv);
			ImGui::SameLine();
			if (ImGui::Button("Set"))
			{
				if (setter_idx == -1)
					sv.assign((char*)src + offset, nullptr);
				else
					ui.set_value(type, src, offset, setter_idx, &sv.v);
			}
			int n = sv.count();
			if (ImGui::InputInt("size", &n, 1, 1))
			{
				n = clamp(n, 0, 16);
				sv.resize(nullptr, n);
			}
			for (auto i = 0; i < n; i++)
			{
				ImGui::PushID(i);
				if (show_variable(ui, ti, str(i), i * ti->size, -1, -1, sv.v.data(), id))
					changed = true;
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		break;
	case TagVU:
		if (ImGui::TreeNode(name.c_str()))
		{
			assert(getter_idx == -1);
			auto pv = (char*)src + offset;
			auto ti = ((TypeInfo_VectorOfUdt*)type)->ti;
			auto& sv = get_staging_vector(id, ti, pv);
			if (ImGui::Button("Get"))
				sv.assign(nullptr, pv);
			ImGui::SameLine();
			if (ImGui::Button("Set"))
			{
				if (setter_idx == -1)
					sv.assign((char*)src + offset, nullptr);
				else
					ui.set_value(type, src, offset, setter_idx, &sv.v);
			}
			auto& ui = *ti->retrive_ui();
			int n = sv.count();
			if (ImGui::InputInt("size", &n, 1, 1))
			{
				n = clamp(n, 0, 16);
				sv.resize(nullptr, n);
			}
			for (auto i = 0; i < n; i++)
			{
				if (i > 0) ImGui::Separator();
				ImGui::PushID(i);
				show_udt(ui, sv.v.data() + ui.size * i);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		break;
	case TagVR:
		if (ImGui::TreeNode(name.c_str()))
		{
			assert(getter_idx == -1);
			auto pv = (char*)src + offset;
			auto ti = ((TypeInfo_VectorOfPair*)type)->ti;
			auto& sv = get_staging_vector(id, ti, pv);
			if (ImGui::Button("Get"))
				sv.assign(nullptr, pv);
			ImGui::SameLine();
			if (ImGui::Button("Set"))
			{
				if (setter_idx == -1)
					sv.assign(pv, nullptr);
				else
					ui.set_value(type, src, offset, setter_idx, &sv.v);
			}
			int n = sv.count();
			if (ImGui::InputInt("size", &n, 1, 1))
			{
				n = clamp(n, 0, 16);
				sv.resize(nullptr, n);
			}
			for (auto i = 0; i < n; i++)
			{
				if (i > 0) ImGui::Separator();
				ImGui::PushID(i);
				auto p = sv.v.data() + ti->size * i;
				show_variable(ui, ti->ti1, "first", 0, -1, -1, ti->first(p), id);
				show_variable(ui, ti->ti2, "second", 0, -1, -1, ti->second(p), id);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		break;
	case TagVT:
		if (ImGui::TreeNode(name.c_str()))
		{
			assert(getter_idx == -1);
			auto pv = (char*)src + offset;
			auto ti = ((TypeInfo_VectorOfTuple*)type)->ti;
			auto& sv = get_staging_vector(id, ti, pv);
			if (ImGui::Button("Get"))
				sv.assign(nullptr, pv);
			ImGui::SameLine();
			if (ImGui::Button("Set"))
			{
				if (setter_idx == -1)
					sv.assign((char*)src + offset, nullptr);
				else
					ui.set_value(type, src, offset, setter_idx, &sv.v);
			}
			int n = sv.count();
			if (ImGui::InputInt("size", &n, 1, 1))
			{
				n = clamp(n, 0, 16);
				sv.resize(nullptr, n);
			}
			for (auto i = 0; i < n; i++)
			{
				if (i > 0) ImGui::Separator();
				ImGui::PushID(i);
				auto p = sv.v.data() + ti->size * i;
				auto j = 0;
				for (auto& t : ti->tis)
				{
					show_variable(ui, t.first, "item_" + str(j), 0, -1, -1, p + t.second, id);
					j++;
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		break;
	}
	ImGui::PopID();

	return changed;
}

std::string show_udt(const UdtInfo& ui, void* src)
{
	std::string changed_name;

	if (ui.attributes.empty())
	{
		for (auto& v : ui.variables)
		{
			if (show_variable(ui, v.type, v.name, v.offset, -1, -1, src, &v))
				changed_name = v.name;
		}
	}
	else
	{
		for (auto& a : ui.attributes)
		{
			if (show_variable(ui, a.type, a.name, a.var_off(), a.getter_idx, a.setter_idx, src, &a))
				changed_name = a.name;
		}
	}

	return changed_name;
}

static std::unordered_map<uint, UdtInfo*> com_udts;
void get_com_udts()
{
	for (auto& ui : tidb.udts)
	{
		if (ui.second.base_class_name == "flame::Component")
			com_udts.emplace(ui.first, &ui.second);
	}
}

void View_Inspector::on_draw()
{
	if (com_udts.empty())
		get_com_udts();

	if (ImGui::Button(graphics::FontAtlas::icon_s("arrow-left"_h).c_str()))
		selection.backward();
	ImGui::SameLine();
	if (ImGui::Button(graphics::FontAtlas::icon_s("arrow-right"_h).c_str()))
		selection.forward();

	static uint sel_ref_type = 0;
	static void* sel_ref_obj = nullptr;
	static auto sel_ref_info = new char[1024];
	if (selection_changed)
	{
		staging_vectors.clear();

		switch (sel_ref_type)
		{
		case th<graphics::Image>():
			graphics::Image::release((graphics::ImagePtr)sel_ref_obj);
			break;
		case th<graphics::Material>():
			graphics::Material::release((graphics::MaterialPtr)sel_ref_obj);
			break;
		case th<graphics::Model>():
			graphics::Model::release((graphics::ModelPtr)sel_ref_obj);
			break;
		case th<graphics::Animation>():
			graphics::Animation::release((graphics::AnimationPtr)sel_ref_obj);
			break;
		}
		sel_ref_type = 0;
		sel_ref_obj = nullptr;
	}

	switch (selection.type)
	{
	case Selection::tEntity:
	{
		auto e = selection.entity();

		ImGui::PushID(e);
		if (e->prefab)
		{
			auto& path = e->prefab->filename;
			auto str = path.string();
			ImGui::InputText("prefab", str.data(), ImGuiInputTextFlags_ReadOnly);
			ImGui::SameLine();
			if (ImGui::Button("P"))
				selection.select(Path::get(path), "inspector"_h);
		}
		auto changed_name = show_udt(*TypeInfo::get<Entity>()->retrive_ui(), e);
		ImGui::PopID();
		if (!changed_name.empty())
		{
			if (auto ins = get_prefab_instance(e); ins)
				ins->mark_modifier(e->file_id, "", changed_name);
		}

		static ComponentPtr target_component = nullptr;
		auto open_component_menu = false;
		for (auto& c : e->components)
		{
			ImGui::PushID(c.get());
			auto& ui = *com_udts[c->type_hash];
			auto open = ImGui::CollapsingHeader(ui.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
			if (ImGui::Button("..."))
			{
				target_component = c.get();
				open_component_menu = true;
			}
			if (open)
			{
				ImGui::Checkbox("enable", &c->enable);
				auto changed_name = show_udt(ui, c.get());
				if (!changed_name.empty())
				{
					if (auto ins = get_prefab_instance(e); ins)
						ins->mark_modifier(e->file_id, ui.name, changed_name);
				}

				if (ui.name == "flame::cNode")
				{
					auto node = (cNodePtr)c.get();
					ImGui::InputFloat4("qut", (float*)&node->qut, "%.3f", ImGuiInputTextFlags_ReadOnly);
					ImGui::InputFloat3("g_pos", (float*)&node->g_pos, "%.3f", ImGuiInputTextFlags_ReadOnly);
					ImGui::InputFloat3("g_rot.x", (float*)&node->g_rot[0], "%.3f", ImGuiInputTextFlags_ReadOnly);
					ImGui::InputFloat3("g_rot.y", (float*)&node->g_rot[1], "%.3f", ImGuiInputTextFlags_ReadOnly);
					ImGui::InputFloat3("g_rot.z", (float*)&node->g_rot[2], "%.3f", ImGuiInputTextFlags_ReadOnly);
					ImGui::InputFloat3("g_scl", (float*)&node->g_scl, "%.3f", ImGuiInputTextFlags_ReadOnly);
				}
				else if (ui.name == "flame::cArmature")
				{
					auto armature = (cArmaturePtr)c.get();
					static char name[100];
					ImGui::InputText("name", name, countof(name));
					ImGui::SameLine();
					if (ImGui::Button("Play"))
						armature->play(sh(name));
					ImGui::SameLine();
					if (ImGui::Button("Stop"))
						armature->stop();
					ImGui::InputFloat("Time", &armature->playing_time, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
					ImGui::DragFloat("Speed", &armature->playing_speed, 0.01f);
				}
				else if (ui.name == "flame::cTerrain")
				{
					auto terrain = (cTerrainPtr)c.get();
					if (ImGui::Button("Procedure Terrain"))
						ProcedureTerrainDialog::open(terrain);
				}
			}
			ImGui::PopID();
		}

		ImGui::Dummy(vec2(0.f, 10.f));
		const float ButtonWidth = 100.f;
		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ButtonWidth) * 0.5f);
		ImGui::SetNextItemWidth(ButtonWidth);
		if (ImGui::Button("Add Component"))
		{
			if (get_prefab_instance(e))
				app.open_message_dialog("[RestructurePrefabInstanceWarnning]", "");
			else
				ImGui::OpenPopup("add_component");
		}

		if (open_component_menu)
		{
			ImGui::OpenPopup("component_menu");
			open_component_menu = false;
		}
		if (ImGui::BeginPopup("component_menu"))
		{
			if (ImGui::Selectable("Remove"))
				e->remove_component(target_component->type_hash);
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopup("add_component"))
		{
			for (auto ui : com_udts)
			{
				if (ImGui::Selectable(ui.second->name.c_str()))
					e->add_component(ui.first);
			}
			ImGui::EndPopup();
		}
	}
		break;
	case Selection::tPath:
	{
		auto& path = selection.path();
		ImGui::TextUnformatted(Path::reverse(path).string().c_str());
		auto ext = path.extension().wstring();
		SUW::to_lower(ext);
		if (ext == L".obj" || ext == L".fbx" || ext == L".gltf" || ext == L".glb")
		{
			static vec3 rotation = vec3(0, 0, 0);
			static vec3 scaling = vec3(0.01f, 0.01f, 0.01f);
			static bool only_animation = false;
			static bool copy_textures = false;
			static std::string texture_fmt = "";
			ImGui::DragFloat3("Rotation", (float*)&rotation);
			ImGui::DragFloat3("Scaling", (float*)&scaling);
			ImGui::Checkbox("Only Animation", &only_animation);
			ImGui::Checkbox("Copy Textures", &copy_textures);
			if (copy_textures)
				ImGui::InputText("Texture Format", &texture_fmt);
			if (ImGui::Button("Convert"))
				graphics::Model::convert(path, rotation, scaling, only_animation, copy_textures, texture_fmt);
		}
		else if (is_image_file(ext))
		{
			struct ImageRefInfo
			{
				uint chs;
				uint bpp;
				bool srgb;
			};
			auto& info = *(ImageRefInfo*)sel_ref_info;

			if (selection_changed)
			{
				auto image = graphics::Image::get(path);
				if (image)
				{
					sel_ref_type = th<graphics::Image>();
					sel_ref_obj = image;

					auto bitmap = Bitmap::create(path);
					info.chs = bitmap->chs;
					info.bpp = bitmap->bpp;
					info.srgb = bitmap->srgb;
					delete bitmap;
				}
			}

			if (sel_ref_obj)
			{
				auto image = (graphics::ImagePtr)sel_ref_obj;
				ImGui::Text("channels: %d", info.chs);
				ImGui::Text("bits per pixel: %d", info.bpp);
				ImGui::Text("srgb: %s", info.srgb ? "yes" : "no");
				ImGui::Text("graphics format: %s", TypeInfo::serialize_t(image->format).c_str());
				ImGui::Text("size: %s", str(image->size).c_str());
				static int view_type = ImGui::ImageViewRGBA;
				static const char* types[] = {
					"RGBA",
					"R", "G", "B", "A",
					"RGB",
				};
				ImGui::Combo("view", &view_type, types, countof(types));
				if (view_type != 0)
					ImGui::PushImageViewType((ImGui::ImageViewType)view_type);
				ImGui::Image(sel_ref_obj, (vec2)image->size);
				if (view_type != 0)
					ImGui::PopImageViewType();
				if (ImGui::Button("Save"))
				{
					image->save(path);
					auto asset = AssetManagemant::find(path);
					if (asset)
						asset->active = false;
				}
			}
		}
		else if (ext == L".fmat")
		{
			if (selection_changed)
			{
				auto material = graphics::Material::get(path);
				if (material)
				{
					sel_ref_type = th<graphics::Material>();
					sel_ref_obj = material;
				}
			}

			if (sel_ref_obj)
			{
				auto material = (graphics::MaterialPtr)sel_ref_obj;
				if (!show_udt(*TypeInfo::get<graphics::Material>()->retrive_ui(), material).empty())
				{
					auto id = app.renderer->get_material_res(material, -2);
					if (id > 0)
						app.renderer->update_res(id, "material"_h, "parameters"_h);
				}
				if (ImGui::Button("Save"))
				{
					material->save(path);
					auto asset = AssetManagemant::find(path);
					if (asset)
						asset->active = false;
				}
			}
		}
		else if (ext == L".fmod")
		{
			if (selection_changed)
			{
				auto model = graphics::Model::get(path);
				if (model)
				{
					sel_ref_type = th<graphics::Model>();
					sel_ref_obj = model;
				}
			}

			if (sel_ref_obj)
			{
				auto model = (graphics::ModelPtr)sel_ref_obj;
				auto i = 0;
				for (auto& mesh : model->meshes)
				{
					ImGui::Text("Mesh %d:", i);
					ImGui::Text("vertex count: %d", mesh.positions.size());
					ImGui::Text("index count: %d", mesh.indices.size());
					std::string attr_str = "";
					if (!mesh.uvs.empty()) attr_str += " uv";
					if (!mesh.normals.empty()) attr_str += " normal";
					if (!mesh.tangents.empty()) attr_str += " tangent";
					if (!mesh.colors.empty()) attr_str += " color";
					if (!mesh.bone_ids.empty()) attr_str += " bone_ids";
					if (!mesh.bone_weights.empty()) attr_str += " bone_weights";
					ImGui::Text("attributes: %s", attr_str.data());
					i++;
				}
			}
		}
		else if (ext == L".fani")
		{
			if (selection_changed)
			{
				auto animation = graphics::Animation::get(path);
				if (animation)
				{
					sel_ref_type = th<graphics::Animation>();
					sel_ref_obj = animation;
				}
			}

			if (sel_ref_obj)
			{
				auto animation = (graphics::AnimationPtr)sel_ref_obj;
				ImGui::Text("Duration: %f", animation->duration);
				if (ImGui::TreeNode(std::format("Channels ({})", (int)animation->channels.size()).c_str()))
				{
					ImGui::BeginChild("channels", ImVec2(0, 0), true);
					for (auto& ch : animation->channels)
					{
						if (ImGui::TreeNode(ch.node_name.c_str()))
						{
							if (ImGui::TreeNode(std::format("  Position Keys({})", (int)ch.position_keys.size()).c_str()))
							{
								for (auto& k : ch.position_keys)
									ImGui::Text("    %f: %s", k.t, str(k.p).c_str());
								ImGui::TreePop();
							}
							if (ImGui::TreeNode(std::format("  Rotation Keys({})", (int)ch.rotation_keys.size()).c_str()))
							{
								for (auto& k : ch.rotation_keys)
									ImGui::Text("    %f: %s", k.t, str((vec4&)k.q).c_str());
								ImGui::TreePop();
							}
							ImGui::TreePop();
						}
					}
					ImGui::EndChild();
					ImGui::TreePop();
				}
			}
		}
	}
		break;
	}

	selection_changed = false;
}
