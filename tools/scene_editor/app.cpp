#include "app.h"
#include "selection.h"
#include "history.h"
#include "scene_window.h"
#include "project_window.h"
#include "inspector_window.h"

#include <flame/xml.h>
#include <flame/foundation/system.h>
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/graphics/model.h>
#include <flame/graphics/shader.h>
#include <flame/graphics/extension.h>
#include <flame/graphics/debug.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/timeline.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/directional_light.h>
#include <flame/universe/systems/renderer.h>

std::vector<Window*> windows;

void View::title_context_menu()
{
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		const auto im_wnd = ImGui::GetCurrentWindow();
		ImRect rect = im_wnd->DockIsActive ? im_wnd->DockTabItemRect : im_wnd->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false))
			ImGui::OpenPopup("title_context_menu");
	}
	if (ImGui::BeginPopup("title_context_menu"))
	{
		if (ImGui::Selectable("New Tab"))
			window->open_view(true);
		ImGui::EndPopup();
	}
}

App app;

struct Preferences
{
	bool use_flame_debugger = false; // use flame visual studio project debugger or use opened project one

};
static Preferences preferences;

static std::filesystem::path preferences_path = L"preferences.ini";

static std::vector<std::function<bool()>> dialogs;

void show_entities_menu()
{
	if (ImGui::MenuItem("New Empty"))
		app.cmd_new_entities(selection.get_entities());
	if (ImGui::BeginMenu("New 3D"))
	{
		if (ImGui::MenuItem("Node"))
			app.cmd_new_entities(selection.get_entities(), "node"_h);
		if (ImGui::MenuItem("Plane"))
			app.cmd_new_entities(selection.get_entities(), "plane"_h);
		if (ImGui::MenuItem("Cube"))
			app.cmd_new_entities(selection.get_entities(), "cube"_h);
		if (ImGui::MenuItem("Sphere"))
			app.cmd_new_entities(selection.get_entities(), "sphere"_h);
		if (ImGui::MenuItem("Cylinder"))
			app.cmd_new_entities(selection.get_entities(), "cylinder"_h);
		if (ImGui::MenuItem("Triangular Prism"))
			app.cmd_new_entities(selection.get_entities(), "tri_prism"_h);
		if (ImGui::MenuItem("Directional Light"))
			app.cmd_new_entities(selection.get_entities(), "dir_light"_h);
		if (ImGui::MenuItem("Point Light"))
			app.cmd_new_entities(selection.get_entities(), "pt_light"_h);
		if (ImGui::MenuItem("Camera"))
			app.cmd_new_entities(selection.get_entities(), "camera"_h);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("New 2D"))
	{
		if (ImGui::MenuItem("Element"))
			app.cmd_new_entities(selection.get_entities(), "element"_h);
		if (ImGui::MenuItem("Image"))
			app.cmd_new_entities(selection.get_entities(), "image"_h);
		if (ImGui::MenuItem("Text"))
			app.cmd_new_entities(selection.get_entities(), "text"_h);
		if (ImGui::MenuItem("Layout"))
			app.cmd_new_entities(selection.get_entities(), "layout"_h);
		ImGui::EndMenu();
	}
	if (ImGui::MenuItem("Duplicate (Shift+D)"))
		app.cmd_duplicate_entities(selection.get_entities());
	if (ImGui::MenuItem("Delete (Del)"))
		app.cmd_delete_entities(selection.get_entities());
}

void open_message_dialog(const std::string& title, const std::string& message)
{
	if (title == "[RestructurePrefabInstanceWarnning]")
	{
		ImGui::OpenMessageDialog("Cannot restructure Prefab Instance",
			"You cannot remove/reposition entities in a Prefab Instance\n"
			"And added entities must be at the end\n"
			"Edit it in that prefab");
	}
	else
		ImGui::OpenMessageDialog(title, message);
}

vec3 App::get_snap_pos(const vec3& _pos)
{
	auto pos = _pos;
	if (move_snap)
	{
		pos /= move_snap_value;
		pos -= fract(pos);
		pos *= move_snap_value;
	}
	return pos;
}

void App::init()
{
	create("Scene Editor", uvec2(800, 600), WindowFrame | WindowResizable, true, graphics_debug, graphics_configs);
	graphics::gui_set_clear(true, vec4(0.f));
	world->update_components = false;
	input->transfer_events = false;
	always_render = false;
	renderer->mode = RenderModeCameraLight;

	auto root = world->root.get();
	root->add_component<cNode>();
	e_editor = Entity::create();
	e_editor->name = "[Editor]";
	e_editor->add_component<cNode>();
	e_editor->add_component<cCamera>();
	root->add_child(e_editor);

	for (auto w : windows)
		w->init();

	auto native_window = main_window->native;
	main_window->native->destroy_listeners.add([this, native_window]() {
		for (auto& p : project_settings.favorites)
			p = Path::reverse(p);
		project_settings.save();

		std::ofstream preferences_o(preferences_path);
		preferences_o << "window_pos=" + str(native_window->pos) << "\n";
		preferences_o << "use_flame_debugger=" + str(preferences.use_flame_debugger) << "\n";
		preferences_o << "[opened_views]\n";
		for (auto w : windows)
		{
			for (auto& v : w->views)
				preferences_o << v->name << "\n";
		}
		if (!project_path.empty())
		{
			preferences_o << "[project_path]\n";
			preferences_o << project_path.string() << "\n";
		}
		if (auto fv = project_window.first_view(); fv && fv->explorer.opened_folder)
		{
			preferences_o << "[opened_folder]\n";
			preferences_o << fv->explorer.opened_folder->path.string() << "\n";
		}
		if (e_prefab)
		{
			preferences_o << "[opened_prefab]\n";
			preferences_o << prefab_path.string() << "\n";
		}
		preferences_o.close();
	}, "app"_h);
}

bool App::on_update()
{
	if (timeline_playing)
	{
		if (opened_timeline && e_timeline_host)
		{
			set_timeline_current_frame((int)timeline_current_frame + 1);
			render_frames++;
		}
		else
			timeline_playing = false;
	}
	return UniverseApplication::on_update();
}

void App::on_gui()
{
	auto last_focused_scene = scene_window.last_focused_view();

	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Project"))
		{
			ImGui::OpenFileDialog("New Project", [this](bool ok, const std::filesystem::path& path) {
				if (ok)
					new_project(path);
			});
		}
		if (ImGui::MenuItem("Open Project"))
		{
			ImGui::OpenFileDialog("Open Project", [this](bool ok, const std::filesystem::path& path) {
				if (ok)
					open_project(path);
			});
		}
		if (ImGui::MenuItem("Close Project"))
			close_project();
		ImGui::Separator();
		if (ImGui::MenuItem("New Prefab"))
		{
			ImGui::OpenFileDialog("New Prefab", [this](bool ok, const std::filesystem::path& path) {
				if (ok)
				{
					new_prefab(path);
					open_prefab(path);
				}
			});
		}
		if (ImGui::MenuItem("Open Prefab"))
		{
			ImGui::OpenFileDialog("Open Prefab", [this](bool ok, const std::filesystem::path& path) {
				if (ok)
					open_prefab(path);
			});
		}
		if (ImGui::MenuItem("Save Prefab (Ctrl+S)"))
			save_prefab();
		if (ImGui::MenuItem("Close Prefab"))
			close_prefab();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo (Ctrl+Z)"))
			cmd_undo();
		if (ImGui::MenuItem("Redo (Ctrl+Y)"))
			cmd_redo();
		if (ImGui::MenuItem(std::format("Clear Histories ({})", (int)histories.size()).c_str()))
		{
			history_idx = -1;
			histories.clear();
		}
		ImGui::Separator();
		show_entities_menu();
		ImGui::Separator();
		if (ImGui::MenuItem("Clear Selections"))
			selection.clear("app"_h);
		if (ImGui::MenuItem("Select Parent"))
			;
		if (ImGui::MenuItem("Select Children"))
			;
		if (ImGui::MenuItem("Invert Siblings"))
			;
		if (ImGui::MenuItem("Focus To Selected (F)"))
		{
			if (last_focused_scene)
				last_focused_scene->focus_to_selected();
		}
		if (ImGui::MenuItem("Selected To Focus (G)"))
		{
			if (last_focused_scene)
				last_focused_scene->selected_to_focus();
		}
		if (ImGui::BeginMenu("Camera"))
		{
			if (ImGui::MenuItem("Reset"))
			{
				if (last_focused_scene)
					last_focused_scene->reset_camera(""_h);
			}
			if (ImGui::MenuItem("X+"))
			{
				if (last_focused_scene)
					last_focused_scene->reset_camera("X+"_h);
			}
			if (ImGui::MenuItem("X-"))
			{
				if (last_focused_scene)
					last_focused_scene->reset_camera("X-"_h);
			}
			if (ImGui::MenuItem("Y+"))
			{
				if (last_focused_scene)
					last_focused_scene->reset_camera("Y+"_h);
			}
			if (ImGui::MenuItem("Y-"))
			{
				if (last_focused_scene)
					last_focused_scene->reset_camera("Y-"_h);
			}
			if (ImGui::MenuItem("Z+"))
			{
				if (last_focused_scene)
					last_focused_scene->reset_camera("Z+"_h);
			}
			if (ImGui::MenuItem("Z-"))
			{
				if (last_focused_scene)
					last_focused_scene->reset_camera("Z-"_h);
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("NavMesh"))
		{
			struct GenerateDialog : ImGui::Dialog
			{
				std::vector<EntityPtr> nodes;
				float agent_radius = 0.6f;
				float agent_height = 1.8f;
				float walkable_climb = 0.5f;
				float walkable_slope_angle = 45.f;

				static void open()
				{
					auto dialog = new GenerateDialog;
					dialog->title = "Generate Navmesh";
					Dialog::open(dialog);
				}

				void draw() override
				{
					bool open = true;
					if (ImGui::Begin(title.c_str(), &open))
					{
						if (ImGui::TreeNode("Nodes"))
						{
							if (ImGui::Button("From Selection"))
							{
								auto entities = selection.get_entities();
								nodes = entities;
							}

							auto n = (int)nodes.size();
							auto size_changed = ImGui::InputInt("size", &n, 1, 1);
							ImGui::Separator();
							if (size_changed)
								nodes.resize(n);
							else
							{
								n = nodes.size();
								for (auto i = 0; i < n; i++)
								{
									ImGui::PushID(i);
									std::string name = nodes[i] ? nodes[i]->name : "[None]";
									ImGui::InputText(nodes[i] ? "" : "Drop Entity Here", &name, ImGuiInputTextFlags_ReadOnly);
									if (ImGui::BeginDragDropTarget())
									{
										if (auto payload = ImGui::AcceptDragDropPayload("Entity"); payload)
										{
											auto entity = *(EntityPtr*)payload->Data;
											nodes[i] = entity;
										}
									}
									ImGui::PopID();
								}
							}
							ImGui::TreePop();
						}
						ImGui::InputFloat("Agent Radius", &agent_radius);
						ImGui::InputFloat("Agent Height", &agent_height);
						ImGui::InputFloat("Walkable Climb", &walkable_climb);
						ImGui::InputFloat("Walkable Slope Angle", &walkable_slope_angle);
						if (ImGui::Button("Generate"))
						{
							sScene::instance()->navmesh_generate(nodes, agent_radius, agent_height, walkable_climb, walkable_slope_angle);
							ImGui::CloseCurrentPopup();
							open = false;
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel"))
						{
							ImGui::CloseCurrentPopup();
							open = false;
						}

						ImGui::End();
					}
					if (!open)
						close();
				}
			};

			if (ImGui::MenuItem("Generate"))
			{
				if (e_prefab)
					GenerateDialog::open();
			}

			if (ImGui::MenuItem("Save"))
			{
				ImGui::OpenFileDialog("Save Navmesh", [](bool ok, const std::filesystem::path& path) {
					if (ok)
						sScene::instance()->navmesh_save(path);
				}, Path::get(L"assets"));
			}

			if (ImGui::MenuItem("Load"))
			{
				ImGui::OpenFileDialog("Load Navmesh", [](bool ok, const std::filesystem::path& path) {
					if (ok)
						sScene::instance()->navmesh_load(path);
				}, Path::get(L"assets"));
			}

			if (ImGui::MenuItem("Export Model"))
			{
				struct ExportDialog : ImGui::Dialog
				{
					std::filesystem::path filename;
					bool merge_vertices = false;
					bool calculate_normals = false;

					static void open()
					{
						auto dialog = new ExportDialog;
						dialog->title = "Navmesh Export Model";
						Dialog::open(dialog);
					}

					void draw() override
					{
						bool open = true;
						if (ImGui::Begin(title.c_str(), &open))
						{
							auto s = filename.string();
							ImGui::InputText("File Name", s.data(), ImGuiInputTextFlags_ReadOnly);
							ImGui::SameLine();
							if (ImGui::Button("..."))
							{
								ImGui::OpenFileDialog("File Name", [this](bool ok, const std::filesystem::path& path) {
									if (ok)
										filename = path;
								}, Path::get(L"assets"));
							}
							ImGui::Checkbox("Merge Vertices", &merge_vertices);
							ImGui::Checkbox("Calculate Normals", &calculate_normals);
							if (ImGui::Button("Export"))
							{
								auto points = sScene::instance()->navmesh_get_mesh();
								if (!points.empty())
								{
									std::vector<uint> indices;
									std::vector<vec3> normals;
									if (merge_vertices)
									{
										// TODO: fix bugs
										//struct Vec3Hasher
										//{
										//	bool operator()(const vec3& a, const vec3& b) const
										//	{
										//		return (std::hash<float>{}(a[0]) ^ std::hash<float>{}(a[1]) ^ std::hash<float>{}(a[2])) <
										//			(std::hash<float>{}(b[0]) ^ std::hash<float>{}(b[1]) ^ std::hash<float>{}(b[2]));
										//	}
										//};

										//std::map<vec3, uint, Vec3Hasher> map;
										//for (auto& p : points)
										//{
										//	auto it = map.find(p);
										//	if (it == map.end())
										//	{
										//		auto index = (uint)map.size();
										//		map[p] = index;
										//		indices.push_back(index);
										//	}
										//	else
										//		indices.push_back(it->second);
										//}
										//points.clear();
										//for (auto& [p, i] : map)
										//	points.push_back(p);
									}
									else
									{
										for (auto i = 0; i < points.size(); i++)
											indices.push_back(i);
									}
									if (calculate_normals)
									{
										normals.resize(points.size());
										for (auto i = 0; i < indices.size(); i += 3)
										{
											auto& a = points[indices[i]];
											auto& b = points[indices[i + 1]];
											auto& c = points[indices[i + 2]];
											auto n = normalize(cross(b - a, c - a));
											normals[indices[i]] += n;
											normals[indices[i + 1]] += n;
											normals[indices[i + 2]] += n;
										}
										for (auto& n : normals)
											n = normalize(n);
									}

									auto model = graphics::Model::create();
									auto& mesh = model->meshes.emplace_back();
									mesh.positions = std::move(points);
									mesh.indices = std::move(indices);
									mesh.normals = std::move(normals);
									model->save(filename);
									delete model;
								}
								ImGui::CloseCurrentPopup();
								open = false;
							}
							ImGui::SameLine();
							if (ImGui::Button("Cancel"))
							{
								ImGui::CloseCurrentPopup();
								open = false;
							}

							ImGui::End();
						}

						if (!open)
							close();
					}
				};

				ExportDialog::open();
			}

			if (ImGui::MenuItem("Test"))
			{
				struct TestDialog : ImGui::Dialog
				{
					vec3 start = vec3(0.f);
					vec3 end = vec3(0.f);
					std::vector<vec3> points;

					static void open()
					{
						auto dialog = new TestDialog;
						dialog->title = "Navmesh Test";
						Dialog::open(dialog);
					}

					void draw() override
					{
						bool open = true;
						if (ImGui::Begin(title.c_str(), &open))
						{
							static int v = 0;
							ImGui::TextUnformatted("use ctrl+click to set start/end");
							ImGui::RadioButton("Start", &v, 0);
							ImGui::TextUnformatted(("    " + str(start)).c_str());
							ImGui::RadioButton("End", &v, 1);
							ImGui::TextUnformatted(("    " + str(end)).c_str());
							if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsKeyDown(Keyboard_Ctrl))
							{
								if (auto fv = scene_window.last_focused_view(); fv)
								{
									if (v == 0)
										start = fv->hovering_pos;
									else
										end = fv->hovering_pos;
								}
								if (distance(start, end) > 0.f)
									points = sScene::instance()->navmesh_query_path(start, end);
							}


							{
								std::vector<vec3> points;
								points.push_back(start - vec3(1, 0, 0));
								points.push_back(start + vec3(1, 0, 0));
								points.push_back(start - vec3(0, 0, 1));
								points.push_back(start + vec3(0, 0, 1));
								sRenderer::instance()->draw_primitives("LineList"_h, points.data(), points.size(), cvec4(0, 255, 0, 255));
							}
							{
								std::vector<vec3> points;
								points.push_back(end - vec3(1, 0, 0));
								points.push_back(end + vec3(1, 0, 0));
								points.push_back(end - vec3(0, 0, 1));
								points.push_back(end + vec3(0, 0, 1));
								sRenderer::instance()->draw_primitives("LineList"_h, points.data(), points.size(), cvec4(0, 0, 255, 255));
							}
							if (!points.empty())
								sRenderer::instance()->draw_primitives("LineList"_h, points.data(), points.size(), cvec4(255, 0, 0, 255));

							ImGui::End();
						}
						if (!open)
							close();
					}
				};

				if (e_prefab)
					TestDialog::open();
			}

			ImGui::EndMenu();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Preferences"))
		{
			struct PreferencesDialog
			{
				bool open = false;
			};
			static PreferencesDialog preferences_dialog;
			dialogs.push_back([&]() {
				if (!preferences_dialog.open)
				{
					preferences_dialog.open = true;
					ImGui::OpenPopup("Preferences");
				}

				if (ImGui::BeginPopupModal("Preferences"))
				{
					ImGui::Checkbox("Use Flame Debugger", &preferences.use_flame_debugger);
					if (ImGui::Button("Close"))
					{
						preferences_dialog.open = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::End();
				}
				return preferences_dialog.open;
				});
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Project"))
	{
		if (ImGui::MenuItem("Open In VS"))
		{
			auto vs_path = get_special_path("Visual Studio Installation Location");
			auto devenv_path = vs_path / L"Common7\\IDE\\devenv.exe";
			auto sln_path = project_path / L"build";
			sln_path = glob_files(sln_path, L".sln")[0];
			exec(devenv_path, std::format(L"\"{}\"", sln_path.wstring()));
		}
		if (ImGui::MenuItem("Attach Debugger"))
			vs_automate({ L"attach_debugger" });
		if (ImGui::MenuItem("Detach Debugger"))
			vs_automate({ L"detach_debugger" });
		if (ImGui::MenuItem("Do CMake"))
			cmake_project();
		if (ImGui::MenuItem("Build (Ctrl+B)"))
			build_project();
		if (ImGui::MenuItem("Clean"))
		{
			if (!project_path.empty())
			{
				auto cpp_path = project_path / L"bin/debug/cpp.dll";
				cpp_path.replace_extension(L".pdb");
				if (std::filesystem::exists(cpp_path))
					std::filesystem::remove(cpp_path);
			}
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Window"))
	{
		for (auto w : windows)
		{
			auto opened = (bool)!w->views.empty();
			if (ImGui::MenuItem(w->name.c_str(), nullptr, opened))
				w->open_view(false);
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Render"))
	{
		if (ImGui::MenuItem("Shaded", nullptr, renderer->mode == RenderModeShaded))
			renderer->mode = RenderModeShaded;
		if (ImGui::MenuItem("Camera Light", nullptr, renderer->mode == RenderModeCameraLight))
			renderer->mode = RenderModeCameraLight;
		if (ImGui::MenuItem("Albedo Data", nullptr, renderer->mode == RenderModeAlbedoData))
			renderer->mode = RenderModeAlbedoData;
		if (ImGui::MenuItem("Normal Data", nullptr, renderer->mode == RenderModeNormalData))
			renderer->mode = RenderModeNormalData;
		if (ImGui::MenuItem("Metallic Data", nullptr, renderer->mode == RenderModeMetallicData))
			renderer->mode = RenderModeMetallicData;
		if (ImGui::MenuItem("Roughness Data", nullptr, renderer->mode == RenderModeRoughnessData))
			renderer->mode = RenderModeRoughnessData;
		if (ImGui::MenuItem("IBL Value", nullptr, renderer->mode == RenderModeIBLValue))
			renderer->mode = RenderModeIBLValue;
		if (ImGui::MenuItem("Fog Value", nullptr, renderer->mode == RenderModeFogValue))
			renderer->mode = RenderModeFogValue;
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Debug"))
	{
		struct UIStatusDialog
		{
			bool open = false;

		};
		static UIStatusDialog ui_status_dialog;

		if (ImGui::MenuItem("UI Status", nullptr, &ui_status_dialog.open))
		{
			if (ui_status_dialog.open)
			{
				dialogs.push_back([&]() {
					if (ui_status_dialog.open)
					{
						ImGui::Begin("UI Status", &ui_status_dialog.open);
						ImGui::Text("Want Capture Mouse: %d", (int)graphics::gui_want_mouse());
						ImGui::Text("Want Capture Keyboard: %d", (int)graphics::gui_want_keyboard());
						ImGui::End();
					}
					return ui_status_dialog.open;
					});
			}
		}
		if (ImGui::MenuItem("Send Debug Cmd"))
		{
			ImGui::OpenInputDialog("Send Debug Cmd", "Cmd", [](bool ok, const std::string& str) {
				if (ok)
					sRenderer::instance()->send_debug_string(str);
				}, "", true);
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
	ImGui::PopStyleVar(2);

	// toolbar begin
	ImGui::Dummy(vec2(0.f, 20.f));
	ImGui::SameLine();
	if (ImGui::ToolButton(graphics::FontAtlas::icon_s("arrow-pointer"_h).c_str(), app.tool == ToolSelect))
		tool = ToolSelect;
	ImGui::SameLine();
	if (ImGui::ToolButton(graphics::FontAtlas::icon_s("arrows-up-down-left-right"_h).c_str(), app.tool == ToolMove))
		tool = ToolMove;
	ImGui::SameLine();
	if (ImGui::ToolButton(graphics::FontAtlas::icon_s("rotate"_h).c_str(), app.tool == ToolRotate))
		tool = ToolRotate;
	ImGui::SameLine();
	if (ImGui::ToolButton(graphics::FontAtlas::icon_s("down-left-and-up-right-to-center"_h).c_str(), app.tool == ToolScale))
		tool = ToolScale;
	ImGui::SameLine();
	const char* tool_pivot_names[] = {
		"Individual",
		"Center"
	};
	const char* tool_mode_names[] = {
		"Local",
		"World"
	};
	ImGui::SetNextItemWidth(100.f);
	ImGui::Combo("##pivot", (int*)&tool_pivot, tool_pivot_names, countof(tool_pivot_names));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	ImGui::Combo("##mode", (int*)&tool_mode, tool_mode_names, countof(tool_mode_names));
	bool* p_snap = nullptr;
	float* p_snap_value = nullptr;
	switch (tool)
	{
	case ToolMove:
		p_snap = &move_snap;
		p_snap_value = last_focused_scene && last_focused_scene->element_targets.empty() ? &move_snap_value : &move_snap_2d_value;
		break;
	case ToolRotate:
		p_snap = &rotate_snap;
		p_snap_value = &rotate_snap_value;
		break;
	case ToolScale:
		p_snap = &scale_snap;
		p_snap_value = &scale_snap_value;
		break;
	}
	ImGui::SameLine();
	if (p_snap)
	{
		ImGui::Checkbox("Snap", p_snap);
		if (*p_snap)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80.f);
			ImGui::InputFloat("##snap_value", p_snap_value);
		}
	}
	ImGui::SameLine();
	if (ImGui::ToolButton(graphics::FontAtlas::icon_s("floppy-disk"_h).c_str()))
		save_prefab();
	ImGui::SameLine();
	ImGui::Dummy(vec2(0.f, 20.f));

	if (selection.type == Selection::tEntity)
	{
		auto e = selection.as_entity();
		if (auto terrain = e->get_component<cTerrain>(); terrain)
		{
			ImGui::SameLine();
			if (ImGui::ToolButton((graphics::FontAtlas::icon_s("mound"_h) + "##up").c_str(), app.tool == ToolTerrainUp))
				tool = ToolTerrainUp;
			ImGui::SameLine();
			if (ImGui::ToolButton((graphics::FontAtlas::icon_s("mound"_h) + "##down").c_str(), app.tool == ToolTerrainDown, 180.f))
				tool = ToolTerrainDown;
			ImGui::SameLine();
			if (ImGui::ToolButton(graphics::FontAtlas::icon_s("paintbrush"_h).c_str(), app.tool == ToolTerrainPaint))
				tool = ToolTerrainPaint;
		}
		if (auto tile_map = e->get_component<cTileMap>(); tile_map)
		{
			ImGui::SameLine();
			if (ImGui::ToolButton(graphics::FontAtlas::icon_s("up-long"_h).c_str(), app.tool == ToolTileMapLevelUp))
				tool = ToolTileMapLevelUp;
			ImGui::SameLine();
			if (ImGui::ToolButton(graphics::FontAtlas::icon_s("down-long"_h).c_str(), app.tool == ToolTileMapLevelDown))
				tool = ToolTileMapLevelDown;
			ImGui::SameLine();
			if (ImGui::ToolButton(graphics::FontAtlas::icon_s("stairs"_h).c_str(), app.tool == ToolTileMapSlope))
				tool = ToolTileMapSlope;
		}
	}

	ImGui::SameLine();
	ImGui::Dummy(vec2(50.f, 20.f));
	ImGui::SameLine();
	if (!e_playing && !e_preview)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
		if (ImGui::ToolButton((graphics::FontAtlas::icon_s("play"_h) + " Build And Play").c_str()))
		{
			build_project();
			add_event([this]() {
				cmd_play();
				return false;
			}, 0.f, 3);
		}
		ImGui::SameLine();
		if (ImGui::ToolButton(graphics::FontAtlas::icon_s("play"_h).c_str()))
			cmd_play();
		ImGui::SameLine();
		if (ImGui::ToolButton(graphics::FontAtlas::icon_s("circle-play"_h).c_str()))
			cmd_start_preview(selection.type == Selection::tEntity ? selection.as_entity() : e_prefab);
		ImGui::PopStyleColor();
	}
	else
	{
		if (e_playing)
		{
			if (!paused)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
				ImGui::SameLine();
				if (ImGui::ToolButton(graphics::FontAtlas::icon_s("pause"_h).c_str()))
					cmd_pause();
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
				ImGui::SameLine();
				if (ImGui::ToolButton(graphics::FontAtlas::icon_s("play"_h).c_str()))
					cmd_play();
				ImGui::PopStyleColor();
			}
		}
		else if (e_preview)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 1, 1));
			ImGui::SameLine();
			if (ImGui::ToolButton(graphics::FontAtlas::icon_s("rotate"_h).c_str()))
				cmd_restart_preview();
			ImGui::PopStyleColor();
		}
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
		ImGui::SameLine();
		if (ImGui::ToolButton(graphics::FontAtlas::icon_s("stop"_h).c_str()))
		{
			if (e_playing)
				cmd_stop();
			else if (e_preview)
				cmd_stop_preview();
		}
		ImGui::PopStyleColor();
		if (e_preview)
		{
			ImGui::SameLine();
			ImGui::Text("[%s]", e_preview->name.c_str());
		}
	}

	// toolbar end

	// dock space
	ImGui::DockSpace(ImGui::GetID("DockSpace"), ImVec2(0.0f, -20.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	// status bar
	ImGui::InvisibleButton("##status_bar", ImGui::GetContentRegionAvail());
	{
		auto dl = ImGui::GetWindowDrawList();
		auto p0 = ImGui::GetItemRectMin();
		auto p1 = ImGui::GetItemRectMax();
		dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
		dl->AddText(p0, ImColor(255, 255, 255, 255), !last_status.empty() ? last_status.c_str() : "OK");
	}
	ImGui::End();

	auto& io = ImGui::GetIO();
	if (ImGui::IsKeyPressed(Keyboard_F5))
	{
		if (!e_playing)
			cmd_play();
		else
		{
			if (e_playing)
				cmd_stop();
			else if (e_preview)
				cmd_stop_preview();
		}
	}
	if (ImGui::IsKeyPressed(Keyboard_F6))
	{
		if (!e_preview)
			cmd_start_preview(selection.type == Selection::tEntity ? selection.as_entity() : e_prefab);
		else
			cmd_restart_preview();
	}
	if (ImGui::IsKeyDown(Keyboard_Ctrl) && ImGui::IsKeyPressed(Keyboard_S))
		save_prefab();
	if (ImGui::IsKeyDown(Keyboard_Ctrl) && ImGui::IsKeyPressed(Keyboard_B))
		build_project();
	if (ImGui::IsKeyDown(Keyboard_Ctrl) && ImGui::IsKeyPressed(Keyboard_Z))
		cmd_undo();
	if (ImGui::IsKeyDown(Keyboard_Ctrl) && ImGui::IsKeyPressed(Keyboard_Y))
		cmd_redo();

	if (e_preview)
	{
		e_preview->forward_traversal([](EntityPtr e) {
			if (!e->global_enable)
				return;
			for (auto& c : e->components)
			{
				if (c->enable)
					c->update();
			}
		});
		render_frames++;
	}

	for (auto it = dialogs.begin(); it != dialogs.end();)
	{
		if (!(*it)())
			it = dialogs.erase(it);
		else
			it++;
	}
}

void App::new_project(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		wprintf(L"cannot create project: %s not exists\n", path.c_str());
		return;
	}
	if (!std::filesystem::is_empty(path))
	{
		wprintf(L"cannot create project: %s is not an empty directory\n", path.c_str());
		return;
	}

	auto project_name = path.filename().string();

	auto assets_path = path / L"assets";
	std::filesystem::create_directories(assets_path);

	auto temp_path = path / L"temp";
	std::filesystem::create_directories(temp_path);

	pugi::xml_document main_prefab;
	{
		auto n_prefab = main_prefab.append_child("prefab");
		n_prefab.append_attribute("file_id").set_value(generate_guid().to_string().c_str());
		n_prefab.append_attribute("name").set_value("Main");
		{
			auto n_components = n_prefab.append_child("components");
			n_components.append_child("item").append_attribute("type_hash").set_value("flame::cNode"_h);
			n_components.append_child("item").append_attribute("type_hash").set_value("cMain"_h);
		}
		{
			auto n_children = n_prefab.append_child("children");
			{
				auto n_item = n_children.append_child("item");
				n_item.append_attribute("file_id").set_value(generate_guid().to_string().c_str());
				n_item.append_attribute("name").set_value("Camera");
				{
					auto n_components = n_item.append_child("components");
					n_components.append_child("item").append_attribute("type_hash").set_value("flame::cNode"_h);
					n_components.append_child("item").append_attribute("type_hash").set_value("flame::cCamera"_h);
				}
			}
		}
	}
	main_prefab.save_file((assets_path / L"main.prefab").c_str());

	auto cpp_path = path / L"cpp";
	std::filesystem::create_directories(cpp_path);

	std::ofstream main_h(cpp_path / L"main.h");
	const auto main_h_content =
		R"^^^(
#pragma once

#include <flame/universe/component.h>

using namespace flame;

FLAME_TYPE(cMain)

// Reflect ctor
struct cMain : Component
{{
	void start() override;

	struct Create
	{{
		virtual cMainPtr operator()(EntityPtr) = 0;
	}};
	// Reflect static
	EXPORT static Create& create;
}};

)^^^";
	main_h << std::format(main_h_content, project_name);
	main_h.close();

	std::ofstream main_cpp(cpp_path / L"main.cpp");
	const auto main_cpp_content =
		R"^^^(
#include <flame/universe/entity.h>

#include "main.h"

void cMain::start()
{{
	printf("Hello World\n");
}}

struct cMainCreate : cMain::Create
{{
	cMainPtr operator()(EntityPtr e) override
	{{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cMain;
	}}
}}cMain_create;
cMain::Create& cMain::create = cMain_create;

EXPORT void* cpp_info()
{{
	auto uinfo = universe_info();
	cMain::create((EntityPtr)INVALID_POINTER);
	return nullptr;
}}

)^^^";
	main_cpp << std::format(main_cpp_content, project_name);
	main_cpp.close();

	std::ofstream app_cpp(path / L"app.cpp");
	const auto app_cpp_content =
		R"^^^(
#include <flame/universe/application.h>

using namespace flame;

UniverseApplication app;

IMPORT void* cpp_info();

int main()
{{
	auto info = cpp_info();
	Path::set_root(L"assets", std::filesystem::current_path() / L"assets");
	app.create(false, "{0}", uvec2(1280, 720), WindowFrame | WindowResizable);
	app.world->root->load(L"assets/main.prefab");
	app.node_renderer->bind_window_targets();
	app.run();
	return 0;
}}

)^^^";
	app_cpp << std::format(app_cpp_content, project_name);
	app_cpp.close();

	auto cmake_path = path / L"CMakeLists.txt";
	std::ofstream cmake_lists(cmake_path);
	const auto cmake_content =
		R"^^^(
cmake_minimum_required(VERSION 3.16.4)
set(flame_path "$ENV{{FLAME_PATH}}")
include("${{flame_path}}/utils.cmake")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_definitions(-W0 -std:c++latest)

project({0})

set_output_dir("${{CMAKE_SOURCE_DIR}}/bin")

set(GLM_INCLUDE_DIR "")
set(IMGUI_DIR "")
file(STRINGS "${{flame_path}}/build/CMakeCache.txt" flame_cmake_cache)
foreach(s ${{flame_cmake_cache}})
	if(GLM_INCLUDE_DIR STREQUAL "")
		string(REGEX MATCH "GLM_INCLUDE_DIR:PATH=(.*)" res "${{s}}")
		if(NOT res STREQUAL "")
			set(GLM_INCLUDE_DIR ${{CMAKE_MATCH_1}})
		endif()
	endif()
	if(IMGUI_DIR STREQUAL "")
		string(REGEX MATCH "IMGUI_DIR:PATH=(.*)" res "${{s}}")
		if(NOT res STREQUAL "")
			set(IMGUI_DIR ${{CMAKE_MATCH_1}})
		endif()
	endif()
endforeach()

file(GLOB_RECURSE source_files "cpp/*.h*" "cpp/*.c*")
add_library(cpp SHARED ${{source_files}})
target_compile_definitions(cpp PUBLIC USE_IMGUI)
target_compile_definitions(cpp PUBLIC "IMPORT=__declspec(dllimport)")
target_compile_definitions(cpp PUBLIC "EXPORT=__declspec(dllexport)")
target_compile_definitions(cpp PUBLIC IMGUI_USER_CONFIG="${config_file}")
target_include_directories(cpp PUBLIC "${{GLM_INCLUDE_DIR}}")
target_include_directories(cpp PUBLIC "${{IMGUI_DIR}}")
target_include_directories(cpp PUBLIC "${{flame_path}}/include")
target_link_libraries(cpp "${{flame_path}}/bin/debug/imgui.lib")
target_link_libraries(cpp "${{flame_path}}/bin/debug/flame_foundation.lib")
target_link_libraries(cpp "${{flame_path}}/bin/debug/flame_graphics.lib")
target_link_libraries(cpp "${{flame_path}}/bin/debug/flame_universe.lib")

file(GENERATE OUTPUT "$<TARGET_FILE_DIR:cpp>/cpp.typedesc" CONTENT "${{CMAKE_CURRENT_SOURCE_DIR}}/cpp" TARGET cpp)
add_custom_command(TARGET cpp POST_BUILD COMMAND "${{flame_path}}/bin/debug/typeinfogen.exe" $<TARGET_FILE:cpp>)

add_executable({0} "app.cpp")
target_link_libraries({0} cpp)
set_target_properties({0} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${{CMAKE_CURRENT_SOURCE_DIR}}")

file(GLOB dll_files "${{flame_path}}/bin/debug/*.dll")
foreach(f IN LISTS dll_files)
	add_custom_command(TARGET {0} POST_BUILD COMMAND ${{CMAKE_COMMAND}} -E copy_if_different ${{f}} $(TargetDir))
endforeach()
file(GLOB typeinfo_files "${{flame_path}}/bin/debug/*.typeinfo")
foreach(f IN LISTS typeinfo_files)
	add_custom_command(TARGET {0} POST_BUILD COMMAND ${{CMAKE_COMMAND}} -E copy_if_different ${{f}} $(TargetDir))
endforeach()

)^^^";
	cmake_lists << std::format(cmake_content, project_name);
	cmake_lists.close();

	auto build_path = path / L"build";
	std::filesystem::create_directories(build_path);
	exec(L"", std::format(L"cmake -S {} -B {}", path.c_str(), build_path.c_str()));
}

void App::open_project(const std::filesystem::path& path)
{
	if (!std::filesystem::is_directory(path))
		return;
	if (e_playing)
		return;

	close_project();

	project_path = path;
	directory_lock(project_path, true);

	auto assets_path = project_path / L"assets";
	if (std::filesystem::exists(assets_path))
	{
		Path::set_root(L"assets", assets_path);
		project_window.reset();
	}
	else
		assert(0);

	project_settings.load(project_path / L"project_settings.xml");
	for (auto& p : project_settings.favorites)
		p = Path::get(p);

	switch (project_settings.build_after_open)
	{
	case 0:
		load_project_cpp();
		break;
	case 1:
		build_project();
		break;
	case 2:
	{
		load_project_cpp();

		struct BuildProjectDialog
		{
			bool open = false;
		};
		static BuildProjectDialog build_project_dialog;
		add_event([]() {
			dialogs.push_back([&]() {
				if (!build_project_dialog.open)
				{
					build_project_dialog.open = true;
					ImGui::OpenPopup("Build Project");
				}

				if (ImGui::BeginPopupModal("Build Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Checkbox("Use Flame Debugger", &preferences.use_flame_debugger);
					if (ImGui::Button("OK"))
					{
						app.build_project();

						ImGui::CloseCurrentPopup();
						build_project_dialog.open = false;
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						ImGui::CloseCurrentPopup();
						build_project_dialog.open = false;
					}
					ImGui::End();
				}

				return build_project_dialog.open;
			});
			return false;
		}, 0.f, 2);
	}
		break;
	}
}

void App::cmake_project()
{
	auto build_path = project_path;
	build_path /= L"build";
	shell_exec(L"cmake", std::format(L"-S \"{}\" -B \"{}\"", project_path.wstring(), build_path.wstring()), true);
}

void App::build_project()
{
	if (project_path.empty())
		return;
	if (e_playing)
	{
		printf("Cannot build project while playing\n");
		open_message_dialog("Build Project", "Cannot build project while playing");
		return;
	}

	auto old_prefab_path = prefab_path;
	if (!old_prefab_path.empty())
		close_prefab();

	add_event([this, old_prefab_path]() {
		unload_project_cpp();

		focus_window(get_console_window());

		cmake_project();

		vs_automate({ L"detach_debugger" });
		auto cpp_project_path = project_path / L"build\\cpp.vcxproj";
		auto vs_path = get_special_path("Visual Studio Installation Location");
		auto msbuild_path = vs_path / L"Msbuild\\Current\\Bin\\MSBuild.exe";
		auto cwd = std::filesystem::current_path();
		std::filesystem::current_path(cpp_project_path.parent_path());
		printf("\n");
		auto cl = std::format(L"\"{}\" {}", msbuild_path.wstring(), cpp_project_path.filename().wstring());
		_wsystem(cl.c_str());
		std::filesystem::current_path(cwd);
		vs_automate({ L"attach_debugger" });
		 
		load_project_cpp();

		if (!old_prefab_path.empty())
			open_prefab(old_prefab_path);

		return false;
	});
}

void App::close_project()
{
	if (e_playing)
		return;

	close_prefab();

	if (!project_path.empty())
		directory_lock(project_path, false);
	project_path = L"";

	Path::set_root(L"assets", L"");
	project_window.reset();
	unload_project_cpp();
}

void App::new_prefab(const std::filesystem::path& path, uint type)
{
	auto e = Entity::create();
	switch (type)
	{
	case "general_3d_scene"_h:
		e->add_component<cNode>();
		auto e_camera = Entity::create();
		e_camera->name = "Camera";
		e_camera->add_component<cNode>();
		e_camera->add_component<cCamera>();
		e->add_child(e_camera);
		auto e_light = Entity::create();
		e_light->name = "Directional Light";
		e_light->add_component<cNode>()->set_eul(vec3(45.f, -60.f, 0.f));
		e_light->add_component<cDirectionalLight>();
		e->add_child(e_light);
		auto e_plane = Entity::create();
		e_plane->name = "Plane";
		e_plane->tag = e_plane->tag;
		e_plane->add_component<cNode>();
		e_plane->add_component<cMesh>()->set_mesh_and_material(L"standard_plane", L"default");
		e->add_child(e_plane);
		break;
	}
	e->save(path);
	delete e;
}

void App::open_prefab(const std::filesystem::path& path)
{
	if (e_playing || ev_open_prefab)
		return;
	close_prefab();
	prefab_path = path;
	prefab_unsaved = false;

	ev_open_prefab = add_event([this]() {
		e_prefab = Entity::create();
		e_prefab->load(prefab_path);
		world->root->add_child(e_prefab);
		ev_open_prefab = nullptr;
		return false;
	});
}

bool App::save_prefab()
{
	if (e_prefab && prefab_unsaved)
	{
		e_prefab->save(prefab_path);
		prefab_unsaved = false;
		last_status = std::format("Prefab Saved : {}", prefab_path.string());
	}
	return true;
}

void App::close_prefab()
{
	if (e_playing)
		return;
	e_preview = nullptr;
	prefab_path = L"";
	selection.clear("app"_h);

	if (e_prefab)
	{
		auto e = e_prefab;
		add_event([this, e]() {
			e->remove_from_parent();
			sScene::instance()->navmesh_clear();
			return false;
		});
		e_prefab = nullptr;
	}
}

void App::load_project_cpp()
{
	auto cpp_path = project_path / L"bin/debug/cpp.dll";
	if (std::filesystem::exists(cpp_path))
		project_cpp_library = tidb.load(cpp_path);
}

void App::unload_project_cpp()
{
	if (project_cpp_library)
	{
		tidb.unload(project_cpp_library);
		project_cpp_library = nullptr;
	}
}

void App::open_timeline(const std::filesystem::path& path)
{
	close_timeline();

	opened_timeline = Timeline::load(path);
}

void App::close_timeline()
{
	if (opened_timeline)
	{
		delete opened_timeline;
		opened_timeline = nullptr;
		timeline_recording = false;
	}
}

void App::set_timeline_host(EntityPtr e)
{
	if (e_timeline_host)
	{
		e_timeline_host->message_listeners.remove("timeline_host"_h);
		e_timeline_host = nullptr;
	}
	if (e)
	{
		e->message_listeners.add([this](uint hash, void*, void*) {
			if (hash == "destroyed"_h)
			{
				e_timeline_host = nullptr;
				if (timeline_recording)
					timeline_recording = false;
			}
		}, "timeline_host"_h);
		e_timeline_host = e;
	}
	timeline_recording = false;
}

void App::set_timeline_current_frame(int frame)
{
	if (timeline_current_frame == frame || frame < 0)
		return;
	timeline_current_frame = frame;
	if (opened_timeline && e_timeline_host)
	{
		auto current_time = frame / 60.f;
		for (auto& t : opened_timeline->tracks)
		{
			float value;
			auto& keyframes = t.keyframes;
			if (keyframes.empty())
				continue;
			auto it = std::lower_bound(keyframes.begin(), keyframes.end(), current_time, [](const auto& a, auto t) {
				return a.time < t;
			});
			if (it == keyframes.end())
				value = s2t<float>(keyframes.back().value);
			else if (it == keyframes.begin())
				value = s2t<float>(keyframes.front().value);
			else
			{
				auto it2 = it - 1;
				auto t1 = it2->time;
				auto t2 = it->time;
				auto v1 = s2t<float>(it2->value);
				auto v2 = s2t<float>(it->value);
				value = mix(v1, v2, (current_time - t1) / (t2 - t1));
			}

			const Attribute* attr = nullptr; void* obj = nullptr; uint component_index;
			resolve_address(t.address, e_timeline_host, attr, obj, component_index);
			if (attr && attr->type->tag == TagD)
			{
				auto ti = (TypeInfo_Data*)attr->type;
				if (component_index < ti->vec_size)
				{
					auto pdata = attr->get_value(obj, true);
					switch (ti->data_type)
					{
					case DataFloat:
						((float*)pdata)[component_index] = value;
						break;
					}
					attr->set_value(obj, pdata);
				}
			}
		}
	}
}

void App::timeline_start_record()
{
	if (timeline_recording)
		return;
	if (e_timeline_host)
		timeline_recording = true;
}

void App::timeline_stop_record()
{
	if (!timeline_recording)
		return;
	timeline_recording = false;
}

KeyframePtr App::get_keyframe(const std::string& address, bool toggle)
{
	auto current_time = timeline_current_frame / 60.f;
	auto it = std::find_if(opened_timeline->tracks.begin(), opened_timeline->tracks.end(), [&](const auto& i) {
		return i.address == address;
	});
	if (it == opened_timeline->tracks.end())
	{
		auto& t = opened_timeline->tracks.emplace_back();
		t.address = address;
		return &t.keyframes.emplace_back(current_time, "");
	}

	auto& t = *it;
	auto it2 = std::find_if(t.keyframes.begin(), t.keyframes.end(), [&](const auto& i) {
		return i.time == current_time;
	});
	if (it2 == t.keyframes.end())
	{
		auto it3 = std::lower_bound(t.keyframes.begin(), t.keyframes.end(), current_time, [&](const auto& i, auto v) {
			return i.time < v;
		});
		return &*t.keyframes.emplace(it3, current_time, "");
	}

	if (toggle)
	{
		t.keyframes.erase(it2);
		if (t.keyframes.empty())
			opened_timeline->tracks.erase(it);
		return nullptr;
	}

	return &*it2;
}

void App::timeline_toggle_playing()
{
	if (timeline_playing)
	{
		timeline_playing = false;
		return;
	}

	if (opened_timeline && e_timeline_host)
		timeline_playing = true;
}

void App::open_file_in_vs(const std::filesystem::path& path)
{
	vs_automate({ L"open_file", path.wstring() });
}

void App::vs_automate(const std::vector<std::wstring>& cl)
{
	std::filesystem::path automation_path = getenv("FLAME_PATH");
	automation_path /= L"bin/debug/vs_automation.exe";
	std::wstring cl_str;
	if (cl[0] == L"attach_debugger" || cl[0] == L"detach_debugger")
	{
		if (!preferences.use_flame_debugger)
			cl_str = L"-p " + project_path.filename().wstring();
		cl_str += L" -c " + cl[0];
		cl_str += L" " + wstr(getpid());
	}
	else if (cl[0] == L"open_file")
	{
		cl_str = L"-p " + project_path.filename().wstring();
		cl_str += L" -c open_file " + cl[1];
	}
	wprintf(L"vs automate: %s\n", cl_str.c_str());
	shell_exec(automation_path, cl_str, true);
}

void App::render_to_image(cCameraPtr camera, graphics::ImageViewPtr dst)
{
	app.world->update_components = true;

	auto previous_camera = app.renderer->camera;
	auto previous_render_mode = app.renderer->mode;
	app.renderer->camera = camera;
	app.renderer->mode = RenderModeCameraLightButNoSky;
	{
		graphics::Debug::start_capture_frame();
		app.renderer->set_targets({ &dst, 1 }, graphics::ImageLayoutShaderReadOnly);
		graphics::InstanceCommandBuffer cb;
		app.renderer->render(0, cb.get());
		cb->image_barrier(dst->image, {}, graphics::ImageLayoutTransferSrc);
		cb.excute();
		graphics::Debug::end_capture_frame();
	}

	app.renderer->camera = previous_camera;
	app.renderer->mode = previous_render_mode;
	if (auto fv = scene_window.first_view(); fv && fv->render_tar)
	{
		auto iv = fv->render_tar->get_view();
		app.renderer->set_targets({ &iv, 1 }, graphics::ImageLayoutShaderReadOnly);
	}
	else
		app.renderer->set_targets({}, graphics::ImageLayoutShaderReadOnly);

	app.world->update_components = false;
}

bool App::cmd_undo()
{
	if (history_idx < 0)
		return false;
	histories[history_idx]->undo();
	history_idx--;
	return true;
}

bool App::cmd_redo()
{
	if (history_idx + 1 >= histories.size())
		return false;
	history_idx++;
	histories[history_idx]->redo();
	return true;
}

bool App::cmd_new_entities(std::vector<EntityPtr>&& ts, uint type)
{
	if (ts.empty())
	{
		if (e_playing)
			ts.push_back(e_playing);
		else if (e_prefab)
			ts.push_back(e_prefab);
		else
			return false;
	}
	std::vector<EntityPtr> es;
	for (auto t : ts)
	{
		auto e = Entity::create();
		e->name = "entity";
		switch (type)
		{
		case "empty"_h:
			break;
		case "node"_h:
			e->add_component<cNode>();
			break;
		case "plane"_h:
			e->add_component<cNode>();
			e->add_component<cMesh>()->set_mesh_and_material(L"standard_plane", L"default");
			break;
		case "cube"_h:
			e->add_component<cNode>();
			e->add_component<cMesh>()->set_mesh_and_material(L"standard_cube", L"default");
			break;
		case "sphere"_h:
			e->add_component<cNode>();
			e->add_component<cMesh>()->set_mesh_and_material(L"standard_sphere", L"default");
			break;
		case "cylinder"_h:
			e->add_component<cNode>();
			e->add_component<cMesh>()->set_mesh_and_material(L"standard_cylinder", L"default");
			break;
		case "tri_prism"_h:
			e->add_component<cNode>();
			e->add_component<cMesh>()->set_mesh_and_material(L"standard_tri_prism", L"default");
			break;
		case "dir_light"_h:
			e->add_component<cNode>()->set_eul(vec3(45.f, -60.f, 0.f));
			e->add_component<cDirectionalLight>();
			break;
		case "pt_light"_h:
			e->add_component<cNode>();
			e->add_component<cPointLight>();
			break;
		case "camera"_h:
			e->add_component<cNode>();
			e->add_component<cCamera>();
			break;
		case "element"_h:
			e->add_component<cElement>();
			break;
		case "image"_h:
			e->add_component<cElement>();
			e->add_component<cImage>();
			break;
		case "text"_h:
			e->add_component<cElement>();
			e->add_component<cText>();
			break;
		case "layout"_h:
			e->add_component<cElement>();
			e->add_component<cLayout>();
			break;
		}
		t->add_child(e);
		es.push_back(e);

		if (auto ins = get_root_prefab_instance(t); ins)
			ins->mark_modification(e->parent->file_id.to_string() + (!e->prefab_instance ? '|' + e->file_id.to_string() : "") + "|add_child");
	}
	if (!e_playing && e_prefab)
	{
		std::vector<GUID> ids(es.size());
		std::vector<GUID> parents(es.size());
		std::vector<uint> indices(es.size());
		std::vector<EntityContent> contents(es.size());
		for (auto i = 0; i < es.size(); i++)
		{
			auto e = es[i];
			ids[i] = e->file_id;
			parents[i] = e->parent->instance_id;
			indices[i] = e->index;
			contents[i].init(e);
		}
		auto h = new EntityHistory(ids, {}, {}, parents, indices, contents);
		add_history(h);
		if (h->ids.size() == 1)
			app.last_status = std::format("Entity Created: {} (type: {})", es[0]->name, type);
		else
			app.last_status = std::format("{} Entities Created: (type: {})", (int)h->ids.size(), type);

		prefab_unsaved = true;
	}
	return true;
}

bool App::cmd_delete_entities(std::vector<EntityPtr>&& es)
{
	if (es.empty())
		return false;
	for (auto e : es)
	{
		if (e == e_prefab)
			return false;
		if (auto ins = get_root_prefab_instance(e); ins && ins != e->prefab_instance.get() &&
			ins->find_modification(e->parent->file_id.to_string() + (!e->prefab_instance ? '|' + e->file_id.to_string() : "") + "|add_child") == -1)
		{
			open_message_dialog("[RestructurePrefabInstanceWarnning]", "");
			return false;
		}
	}
	
	std::vector<std::string> names;
	std::vector<GUID> ids;
	std::vector<GUID> parents;
	std::vector<uint> indices;
	std::vector<EntityContent> contents;
	if (!e_playing && e_prefab)
	{
		names.resize(es.size());
		ids.resize(es.size());
		parents.resize(es.size());
		indices.resize(es.size());
		contents.resize(es.size());
	}
	for (auto i = 0; i < es.size(); i++)
	{
		auto e = es[i];
		names[i] = e->name;
		add_event([e]() {
			if (auto ins = get_root_prefab_instance(e); ins)
				ins->remove_modification(e->parent->file_id.to_string() + (!e->prefab_instance ? '|' + e->file_id.to_string() : "") + "|add_child");
			e->remove_from_parent();
			return false;
		});

		if (e_prefab)
		{
			ids[i] = e->file_id;
			parents[i] = e->parent->instance_id;
			indices[i] = e->index;
			contents[i].init(e);
		}
	}

	auto is_selecting_entities = selection.type == Selection::tEntity;
	auto selected_entities = selection.get_entities();
	for (auto e : es)
	{
		if (is_selecting_entities)
		{
			if (auto it = std::find(selected_entities.begin(), selected_entities.end(), e); it != selected_entities.end())
				selected_entities.erase(it);
		}
	}
	if (is_selecting_entities)
		selection.select(selected_entities, "app"_h);

	if (!e_playing && e_prefab)
	{
		auto h = new EntityHistory(ids, {}, {}, parents, indices, contents);
		add_history(h);
		if (h->ids.size() == 1)
			app.last_status = std::format("Entity Removed: {}", names[0]);
		else
			app.last_status = std::format("{} Entities Removed", (int)h->ids.size());

		prefab_unsaved = true;
	}
	return true;
}

bool App::cmd_duplicate_entities(std::vector<EntityPtr>&& es)
{
	if (es.empty())
		return false;
	for (auto t : es)
	{
		if (t == e_prefab)
			return false;
	}
	std::vector<EntityPtr> new_entities;
	for (auto t : es)
	{
		auto new_one = t->duplicate();
		new_entities.push_back(new_one);
		t->parent->add_child(new_one);

		if (auto ins = get_root_prefab_instance(new_one->parent); ins)
			ins->mark_modification(new_one->parent->file_id.to_string() + (!new_one->prefab_instance ? '|' + new_one->file_id.to_string() : "") + "|add_child");
	}
	selection.select(new_entities, "app"_h);

	if (!e_playing && e_prefab)
	{
		std::vector<GUID> ids(new_entities.size());
		std::vector<GUID> parents(new_entities.size());
		std::vector<uint> indices(new_entities.size());
		std::vector<EntityContent> contents(new_entities.size());
		for (auto i = 0; i < new_entities.size(); i++)
		{
			auto e = new_entities[i];
			ids[i] = e->file_id;
			parents[i] = e->parent->instance_id;
			indices[i] = e->index;
			contents[i].init(e);
		}
		auto h = new EntityHistory(ids, {}, {}, parents, indices, contents);
		add_history(h);
		if (h->ids.size() == 1)
			app.last_status = std::format("Entity Dyplicated: {}", new_entities[0]->name);
		else
			app.last_status = std::format("{} Entities Dyplicated", (int)h->ids.size());

		prefab_unsaved = true;
	}
	return true;
}

bool App::cmd_play()
{
	if (e_preview)
		return false;
	if (!e_playing && e_prefab)
	{
		add_event([this]() {
			e_prefab->remove_from_parent(false);
			e_playing = e_prefab->duplicate();
			world->root->add_child(e_playing);
			world->update_components = true;
			input->transfer_events = true;
			always_render = true;
			paused = false;

			auto fv = scene_window.last_focused_view();
			auto& camera_list = cCamera::list();
			if (camera_list.size() > 1)
			{
				if (fv)
					fv->camera_idx = 1;
			}
			return false;
		});
		return true;
	}
	else if (paused)
	{
		paused = false;
		world->update_components = true;
		input->transfer_events = true;
		return true;
	}
	return false;
}

bool App::cmd_pause()
{
	if (!e_playing || paused)
		return false;
	paused = true;
	world->update_components = false;
	input->transfer_events = false;
	return true;
}

bool App::cmd_stop()
{
	if (!e_playing)
		return false;
	add_event([this]() {
		e_playing->remove_from_parent();
		e_playing = nullptr;
		world->root->add_child(e_prefab);
		world->update_components = false;
		always_render = false;

		auto fv = scene_window.last_focused_view();
		auto& camera_list = cCamera::list();
		if (camera_list.size() > 0)
		{
			if (fv)
				fv->camera_idx = 0;
			sRenderer::instance()->camera = camera_list.front();
		}
		else
		{
			if (fv)
				fv->camera_idx = -1;
			sRenderer::instance()->camera = nullptr;
		}
		return false;
	});

	return true;
}

bool App::cmd_start_preview(EntityPtr e)
{
	if (e_preview)
		cmd_stop_preview();

	e_preview = e;

	if (e_preview->enable)
	{
		e_preview->set_enable(false);
		e_preview->set_enable(true);
	}
	e_preview->forward_traversal([](EntityPtr e) {
		if (!e->global_enable)
			return;
		for (auto& c : e->components)
		{
			if (c->enable)
				c->start();
		}
	});

	return true;
}

bool App::cmd_stop_preview()
{
	if (!e_preview)
		return false;

	if (e_preview->enable)
	{
		e_preview->set_enable(false);
		e_preview->set_enable(true);
	}

	e_preview = nullptr;

	return true;
}

bool App::cmd_restart_preview()
{
	if (!e_preview)
		return false;

	auto e = e_preview;
	cmd_stop_preview();
	cmd_start_preview(e);

	return true;
}

int main(int argc, char** args)
{
	srand(time(0));

	auto ap = parse_args(argc, args);
	if (ap.has("-fixed_render_target_size"))
		scene_window.fixed_render_target_size = true;
	if (ap.has("-dont_use_mesh_shader"))
		app.graphics_configs.emplace_back("mesh_shader"_h, 0);
	if (ap.has("-replace_renderpass_attachment_dont_care_to_load"))
		app.graphics_configs.emplace_back("replace_renderpass_attachment_dont_care_to_load"_h, 1);

	app.init();

	auto preferences_i = parse_ini_file(preferences_path);
	for (auto& e : preferences_i.get_section_entries(""))
	{
		if (e.key == "window_pos")
		{
			auto pos = s2t<2, int>(e.values[0]);
			auto screen_size = get_screen_size();
			auto num_monitors = get_num_monitors();
			if (pos.x >= screen_size.x * num_monitors)
				pos.x = 0;
			app.main_window->native->set_pos(pos);
		}
		if (e.key == "use_flame_debugger")
			preferences.use_flame_debugger = s2t<bool>(e.values[0]);
	}
	for (auto& e : preferences_i.get_section_entries("opened_views"))
	{
		for (auto w : windows)
		{
			auto name = e.values[0];
			auto sp = SUS::split(name, '#');
			if (w->name == sp.front())
			{
				w->open_view(name);
				break;
			}
		}
	}
	for (auto& e : preferences_i.get_section_entries("project_path"))
	{
		app.open_project(e.values[0]);
		break;
	}
	for (auto& e : preferences_i.get_section_entries("opened_folder"))
	{
		if (auto v = project_window.views.empty() ? nullptr : project_window.views.front().get(); v)
			((ProjectView*)v)->explorer.peeding_open_path = e.values[0];
		break;
	}
	for (auto& e : preferences_i.get_section_entries("opened_prefab"))
	{
		app.open_prefab(e.values[0]);
		break;
	}

	app.run();

	return 0;
}
