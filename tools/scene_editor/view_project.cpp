#include "selection.h"
#include "view_project.h"
#include "view_scene.h"

#include <flame/foundation/bitmap.h>
#include <flame/foundation/system.h>
#include <flame/foundation/typeinfo.h>
#include <flame/graphics/extension.h>
#include <flame/graphics/material.h>
#include <flame/graphics/model.h>
#include <flame/graphics/shader.h>
#include <flame/graphics/debug.h>

View_Project view_project;
static auto selection_changed = false;

View_Project::View_Project() :
	GuiView("Project")
{
	selection.callbacks.add([](uint caller) {
		if (caller != "project"_h)
			selection_changed = true;
	}, "project"_h);
}

void View_Project::reset(const std::filesystem::path& assets_path)
{
	explorer.reset(assets_path);

	if (flame_file_watcher)
	{
		set_native_event(flame_file_watcher);
		flame_file_watcher = nullptr;
	}
	if (assets_file_watcher)
	{
		set_native_event(assets_file_watcher);
		assets_file_watcher = nullptr;
	}
	auto file_watcher = [this](FileChangeFlags flags, const std::filesystem::path& path) {
		mtx_changed_paths.lock();
		auto it = changed_paths.find(path);
		if (it == changed_paths.end())
			changed_paths[path] = flags;
		else
			it->second = (FileChangeFlags)(it->second | flags);
		mtx_changed_paths.unlock();
	};
	flame_file_watcher = add_file_watcher(Path::get(L"flame"), file_watcher, true, false);
	assets_file_watcher = add_file_watcher(assets_path, file_watcher, true, false);
}

std::filesystem::path find_free_filename(const std::filesystem::path& prefix, const std::filesystem::path& ext = L"")
{
	auto i = 0;
	auto p = prefix;
	p += wstr(i);
	p += ext;
	while (std::filesystem::exists(p))
	{
		i++;
		p = prefix;
		p += wstr(i);
		p += ext;
	}
	return p;
}

void View_Project::init()
{
	explorer.select_callback = [this](const std::filesystem::path& path) {
		if (path.empty())
			selection.clear("project"_h);
		else
			selection.select(path, "project"_h);
	};
	explorer.dbclick_callback = [this](const std::filesystem::path& path) {
		auto ext = path.extension();
		if (ext == L".prefab")
			app.open_prefab(path);
	};
	explorer.item_context_menu_callback = [this](const std::filesystem::path& path) {
		auto ext = path.extension();
		if (ImGui::MenuItem("Show In Explorer"))
			exec(L"", std::format(L"explorer /select,\"{}\"", path.wstring()));
		if (is_text_file(ext))
		{
			if (ImGui::MenuItem("Open In VS"))
			{
				auto vs_path = get_special_path("Visual Studio Installation Location");
				if (!vs_path.empty())
					exec(vs_path / L"Common7\\IDE\\devenv.exe", std::format(L" /edit \"{}\"", path.wstring()));
			}
		}
		if (ImGui::BeginMenu("Copy Path"))
		{
			if (ImGui::MenuItem("Name"))
				set_clipboard(path.filename().wstring());
			if (ImGui::MenuItem("Path"))
				set_clipboard(Path::reverse(path).wstring());
			if (ImGui::MenuItem("Absolute Path"))
				set_clipboard(path.wstring());
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Rename"))
		{
			ImGui::OpenInputDialog("New name", [path](bool ok, const std::string& text) {
				if (ok)
				{
					auto new_name = path;
					new_name.replace_filename(text);
					std::error_code ec;
					std::filesystem::rename(path, new_name, ec);
				}
			}, path.filename().stem().string());
		}
		if (ImGui::MenuItem("Delete"))
		{
			ImGui::OpenYesNoDialog(std::format("Are you sure to delete \"{}\" ?", path.string()), [path](bool yes) {
				if (yes)
				{
					std::error_code ec;
					std::filesystem::remove(path, ec);
				}
			});
		}
	};
	explorer.folder_context_menu_callback = [this](const std::filesystem::path& path) {
		if (ImGui::MenuItem("Show In Explorer"))
			exec(L"", std::format(L"explorer /select,\"{}\"", path.wstring()));
		if (ImGui::MenuItem("New Folder"))
			std::filesystem::create_directory(find_free_filename(path / L"new_foler_"));
		if (ImGui::MenuItem("New Image"))
		{
			struct NewImageDialog : ImGui::Dialog
			{
				std::filesystem::path dir;
				std::string name = "new_image";
				int format = 0;
				ivec2 size = ivec2(256);
				int type = 0;
				vec2 noise_offset = vec2(3.8f, 7.5f);
				float noise_scale = 4.f;
				float noise_falloff = 10.f;
				float noise_power = 3.f;

				std::unique_ptr<graphics::Image> image;

				static void open(const std::filesystem::path& dir)
				{
					auto dialog = new NewImageDialog;
					dialog->title = "New Image";
					dialog->dir = dir;
					Dialog::open(dialog);
				}

				graphics::ImagePtr generate_image()
				{
					if (size.x <= 0 || size.y <= 0)
						return nullptr;

					graphics::Format fmt;
					switch (format)
					{
					case 0: fmt = graphics::Format_R8G8B8A8_UNORM; break;
					case 1: fmt = graphics::Format_R8_UNORM; break;
					}

					auto ret = graphics::Image::create(fmt, (uvec2)size, graphics::ImageUsageTransferSrc | graphics::ImageUsageAttachment | graphics::ImageUsageSampled);

					graphics::InstanceCommandBuffer cb;
					cb->image_barrier(ret, {}, graphics::ImageLayoutAttachment);
					cb->set_viewport_and_scissor(Rect(vec2(0), vec2(size)));
					switch (type)
					{
					case 0:
						cb->begin_renderpass(nullptr, ret->get_shader_write_dst(0, 0, graphics::AttachmentLoadClear), { vec4(0.f) });
						cb->end_renderpass();
						break;
					case 1:
						cb->begin_renderpass(nullptr, ret->get_shader_write_dst(0, 0, graphics::AttachmentLoadClear), { vec4(1.f) });
						cb->end_renderpass();
						break;
					case 2:
					{
						auto fb = ret->get_shader_write_dst();
						auto pl = graphics::GraphicsPipeline::get(L"flame\\shaders\\noise\\fbm.pipeline",
							{ "rp=" + str(fb->renderpass) });
						graphics::PipelineResourceManager prm;
						prm.init(pl->layout);

						cb->begin_renderpass(nullptr, fb);
						cb->bind_pipeline(pl);
						prm.pc.item("uv_off"_h).set(noise_offset);
						prm.pc.item("uv_scl"_h).set(noise_scale);
						prm.pc.item("val_base"_h).set(0.f);
						prm.pc.item("val_scl"_h).set(1.f);
						prm.pc.item("falloff"_h).set(1.f / clamp(noise_falloff, 2.f, 100.f));
						prm.pc.item("power"_h).set(noise_power);
						prm.push_constant(cb.get());
						cb->draw(3, 1, 0, 0);
						cb->end_renderpass();
					}
					break;
					}
					cb->image_barrier(ret, {}, graphics::ImageLayoutShaderReadOnly);
					cb.excute();

					return ret;
				}

				void draw() override
				{
					bool open = true;
					if (ImGui::Begin(title.c_str(), &open))
					{
						ImGui::InputText("name", &name);
						static const char* formats[] = {
							"RGBA8",
							"R8"
						};
						ImGui::Combo("format", &format, formats, countof(formats));
						ImGui::InputInt2("size", &size[0]);
						static const char* types[] = {
							"Black",
							"White",
							"Fbm"
						};
						ImGui::Combo("type", &type, types, countof(types));
						switch (type)
						{
						case 2:
							ImGui::DragFloat2("offset", (float*)&noise_offset, 0.1f, 0.f, 100.f);
							ImGui::DragFloat("scale", &noise_scale, 0.1f, 0.f, 10.f);
							ImGui::DragFloat("falloff", &noise_falloff, 1.f, 2.f, 100.f);
							ImGui::DragFloat("power", &noise_power, 0.01f, 1.f, 10.f);
							break;
						}
						if (ImGui::Button("Generate"))
						{
							graphics::Queue::get()->wait_idle();
							image.reset(generate_image());
						}
						if (image)
							ImGui::Image(image.get(), (vec2)size);
						if (ImGui::Button("Save"))
						{
							if (image && !name.empty())
								image->save(dir / name);
						}
						ImGui::SameLine();
						if (ImGui::Button("Close"))
							close();

						ImGui::End();
					}
					if (!open)
						close();
				}
			};

			NewImageDialog::open(path);
		}
		if (ImGui::MenuItem("New Material"))
		{
			auto material = graphics::Material::create();
			material->save(find_free_filename(path / L"new_material_", L".fmat"));
		}
		if (ImGui::MenuItem("New Prefab"))
			app.new_prefab(find_free_filename(path / L"new_prefab_", L".prefab"));
	};
	explorer.folder_drop_callback = [this](const std::filesystem::path& path) {
		if (auto payload = ImGui::AcceptDragDropPayload("Entity"); payload)
		{
			auto e_src = *(EntityPtr*)payload->Data;
			if (e_src->prefab_instance)
				app.open_message_dialog("Error", "Entity is already a prefab instance");
			else
			{
				auto fn = std::filesystem::path(e_src->name);
				if (fn.empty())
					fn = L"entity";
				fn += L".prefab";
				fn = path / fn;
				while (true)
				{
					if (!std::filesystem::exists(fn))
						break;
					replace_fn(fn, L"{}_");
				}
				new PrefabInstance(e_src, fn);
				e_src->save(fn);
			}
		}
	};
}

void View_Project::on_draw()
{
	mtx_changed_paths.lock();
	if (!changed_paths.empty())
	{
		std::vector<std::filesystem::path> changed_directories;
		std::vector<std::pair<std::filesystem::path, FileChangeFlags>> changed_files;
		for (auto& p : changed_paths)
		{
			if (std::filesystem::is_directory(p.first) && p.second == FileModified)
				changed_directories.push_back(p.first);
			else
				changed_files.emplace_back(p.first, p.second);

			if (p.second != FileModified)
			{
				auto parent_path = p.first.parent_path();
				auto found = false;
				for (auto& pp : changed_directories)
				{
					if (pp == parent_path)
					{
						found = true;
						break;
					}
				}
				if (!found)
					changed_directories.push_back(parent_path);
			}
		}
		std::sort(changed_directories.begin(), changed_directories.end(), [](const auto& a, const auto& b) {
			return a.wstring().size() < b.wstring().size();
		});
		std::sort(changed_files.begin(), changed_files.end(), [](const auto& a, const auto& b) {
			return a.first.wstring().size() < b.first.wstring().size();
		});

		auto current_path = explorer.opened_folder ? explorer.opened_folder->path : L"";

		for (auto& p : changed_directories)
		{
			if (auto node = explorer.find_folder(p); node && node->read)
			{
				node->read = false;
				node->read_children();
			}
		}

		explorer.open_folder(current_path.empty() ? nullptr : explorer.find_folder(current_path));

		std::vector<std::pair<AssetManagemant::Asset*, std::filesystem::path>>	changed_assets;
		std::pair<std::vector<graphics::ShaderPtr>, uint>						shaders;
		std::pair<std::vector<graphics::GraphicsPipelinePtr>, uint>				graphics_pipelines;
		std::vector<graphics::ShaderPtr>										changed_shaders;
		std::vector<graphics::GraphicsPipelinePtr>								changed_pipelines;
		auto get_shaders = [&]() {
			if (shaders.second < frames)
			{
				shaders.first = graphics::Debug::get_shaders();
				shaders.second = frames;
			}
		};
		auto get_graphics_pipelines = [&]() {
			if (graphics_pipelines.second < frames)
			{
				graphics_pipelines.first = graphics::Debug::get_graphics_pipelines();
				graphics_pipelines.second = frames;
			}
		};
		for (auto& p : changed_files)
		{
			if ((p.second & FileModified) || (p.second & FileRemoved) || (p.second & FileRenamed))
			{
				if (auto asset = AssetManagemant::find(p.first); asset)
				{
					if (asset->active)
						changed_assets.emplace_back(asset, p.first);
					else
						asset->active = true;
				}
				auto ext = p.first.extension();
				if (ext == L".glsl" || ext == L".vert" || ext == L".frag" || ext == L".tesc" || ext == L".tese" || ext == L".geom")
				{
					get_shaders();
					for (auto sd : shaders.first)
					{
						if (sd->filename == p.first)
							changed_shaders.push_back(sd);
					}
				}
				else if (ext == L".pipeline")
				{
					get_graphics_pipelines();
					for (auto pl : graphics_pipelines.first)
					{
						if (pl->filename == p.first)
							changed_pipelines.push_back(pl);
					}
				}
			}
		}
		if (!changed_assets.empty())
		{
			if (app.e_prefab)
			{
				std::vector<std::tuple<void*, Attribute*, std::filesystem::path>> affected_attributes;
				app.e_prefab->forward_traversal([&](EntityPtr e) {
					for (auto& c : e->components)
					{
						auto& ui = *find_udt(c->type_hash);
						for (auto& a : ui.attributes)
						{
							if (a.type == TypeInfo::get<std::filesystem::path>())
							{
								auto value = std::filesystem::path(a.serialize(c.get()));
								auto abs_value = Path::get(value);
								for (auto& asset : changed_assets)
								{
									if (abs_value == asset.second)
									{
										std::filesystem::path p;
										a.set_value(c.get(), &p);
										affected_attributes.emplace_back(c.get(), &a, value);
										break;
									}
								}
							}
						}
					}
				});
				for (auto& a : affected_attributes)
					std::get<1>(a)->set_value(std::get<0>(a), &std::get<2>(a));
			}
		}
		if (!changed_shaders.empty())
		{
			get_graphics_pipelines();
			for (auto sd : changed_shaders)
			{
				for (auto pl : graphics_pipelines.first)
				{
					for (auto _sd : pl->shaders)
					{
						if (sd == _sd)
						{
							auto found = false;
							for (auto _pl : changed_pipelines)
							{
								if (pl == _pl)
								{
									found = true;
									break;
								}
							}
							if (!found)
								changed_pipelines.push_back(pl);
							break;
						}
					}
				}
			}
		}
		if (!changed_shaders.empty() || !changed_pipelines.empty())
			graphics::Queue::get()->wait_idle();
		if (!changed_shaders.empty())
		{
			for (auto sd : changed_shaders)
				sd->recreate();
		}
		if (!changed_pipelines.empty())
		{
			for (auto pl : changed_pipelines)
				pl->recreate();
		}

		changed_paths.clear();
	}
	mtx_changed_paths.unlock();

	if (selection.type == Selection::tPath)
	{
		if (selection_changed)
			explorer.ping(selection.path());
		explorer.selected_path = selection.path();
	}
	else
		explorer.selected_path.clear();

	explorer.draw();

	selection_changed = false;
}
