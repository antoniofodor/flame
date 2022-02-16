#include "app.h"
#include "selection.h"
#include "view_scene.h"
#include "view_project.h"

#include <flame/foundation/system.h>

std::list<View*> views;

View::View(std::string_view name) :
	name(name)
{
	views.push_back(this);
}

void View::open()
{
	if (lis)
		return;

	lis = app.main_window->imgui_callbacks.add([this]() {
		draw();
	});
}

void View::close()
{
	if (!lis)
		return;

	add_event([this]() {
		app.main_window->imgui_callbacks.remove(lis);
		return false;
	});
	lis = nullptr;
}

void View::draw()
{
	bool open = true;
	ImGui::Begin(name.c_str(), &open);
	on_draw();
	ImGui::End();

	if (!open)
		close();
}

App app;

void App::init()
{
	app.create(true, "Scene Editor", uvec2(1280, 720), WindowFrame | WindowResizable | WindowMaximized);
	app.always_render = false;

	auto root = app.world->root.get();
	root->add_component(th<cNode>());
	e_editor = Entity::create();
	e_editor->name = "[Editor]";
	e_editor->add_component(th<cNode>());
	e_editor->add_component(th<cCamera>());
	root->add_child(e_editor);

	app.main_window->imgui_callbacks.add([this]() {
		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Project"))
			{
				#ifdef USE_IM_FILE_DIALOG
				ifd::FileDialog::Instance().Open("OpenProjectDialog", "Open a project", "");
				#endif
				;
			}
			if (ImGui::MenuItem("Open Prefab"))
			{
				#ifdef USE_IM_FILE_DIALOG
				ifd::FileDialog::Instance().Open("OpenPrefabDialog", "Open a prefab", "Prefab file (*.prefab){.prefab}");
				#endif
				;
			}
			if (ImGui::MenuItem("New Prefab"))
			{
				#ifdef USE_IM_FILE_DIALOG
				ifd::FileDialog::Instance().Save("NewPrefabDialog", "New prefab", "Prefab file (*.prefab){.prefab}");
				#endif
				;
			}
			if (ImGui::MenuItem("Save Prefab"))
			{
				if (e_prefab)
					e_prefab->save(prefab_path);
			}
			if (ImGui::MenuItem("Close"))
				;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Scene"))
		{
			if (ImGui::MenuItem("Create Entity"))
				cmd_create_entity();
			if (ImGui::MenuItem("Delete Entity"))
				cmd_delete_selected_entity();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			for (auto w : views)
			{
				auto selected = (bool)w->lis;
				if (ImGui::MenuItem(w->name.c_str(), nullptr, &selected))
					w->open();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Render"))
		{
			if (ImGui::MenuItem("Always Render", nullptr, app.always_update))
			{
				app.always_update = !app.always_update;
				//app.s_renderer->set_always_update(app.always_update);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();

#ifdef USE_IM_FILE_DIALOG
		if (ifd::FileDialog::Instance().IsDone("OpenProjectDialog"))
		{
			if (ifd::FileDialog::Instance().HasResult())
				app.open_project(ifd::FileDialog::Instance().GetResultFormated());
			ifd::FileDialog::Instance().Close();
		}
		if (ifd::FileDialog::Instance().IsDone("OpenPrefabDialog"))
		{
			if (ifd::FileDialog::Instance().HasResult())
				open_prefab(ifd::FileDialog::Instance().GetResultFormated());
			ifd::FileDialog::Instance().Close();
		}
		if (ifd::FileDialog::Instance().IsDone("NewPrefabDialog"))
		{
			if (ifd::FileDialog::Instance().HasResult())
			{
				auto path = ifd::FileDialog::Instance().GetResultFormated();
				auto e = Entity::create();
				e->name = "Hello";
				e->save(path);
				open_prefab(path);
			}
			ifd::FileDialog::Instance().Close();
		}
#endif

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
		ImGui::DockSpace(ImGui::GetID("DockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		ImGui::End();

		if (open_message_dialog)
		{
			ImGui::OpenPopup(message_dialog_title.c_str());
			open_message_dialog = false;
		}
		if (ImGui::BeginPopupModal(message_dialog_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted(message_dialog_text.c_str());
			if (ImGui::Button("OK"))
			{
				message_dialog_title.clear();
				message_dialog_text.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	});
}

void App::open_project(const std::filesystem::path& path)
{
	if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
	{
		if (!project_path.empty())
			directory_lock(project_path, false);

		selection.clear();
		project_path = path;
		directory_lock(project_path, true);

		auto assets_path = path / L"assets";
		if (!std::filesystem::exists(assets_path))
			std::filesystem::create_directories(assets_path);
		Path::set_root(L"assets", assets_path);

		auto cmake_path = path / L"CMakeLists.txt";
		if (!std::filesystem::exists(cmake_path))
		{
			auto project_name = path.filename().string();
			std::ofstream cmake_lists(cmake_path);
			cmake_lists << "cmake_minimum_required(VERSION 3.16.4)" << std::endl;
			cmake_lists << "set_property(GLOBAL PROPERTY USE_FOLDERS ON)" << std::endl;
			cmake_lists << "add_definitions(-W0 -std:c++latest)" << std::endl;
			cmake_lists << std::format("project({})", project_name) << std::endl;
			cmake_lists << "file(GLOB_RECURSE source_files \"*.h*\" \"*.c*\")" << std::endl;
			cmake_lists << std::format("add_executable({} ${{source_files}})", project_name) << std::endl;
			cmake_lists.close();
		}

		auto build_path = path / L"build";
		if (!std::filesystem::exists(build_path))
		{
			std::filesystem::create_directories(build_path);
			exec(L"", std::format(L"cmake -S {} -B {}", path.c_str(), build_path.c_str()));
		}

		view_project.reset();
	}
}

void App::open_prefab(const std::filesystem::path& path)
{
	prefab_path = path;

	if (e_prefab)
		e_prefab->parent->remove_child(e_prefab);
	e_prefab = Entity::create();
	e_prefab->load(path);
	app.world->root->add_child(e_prefab);
}

bool App::cmd_create_entity()
{
	if (!e_prefab)
		return false;
	static int id = 0;
	auto e = Entity::create();
	e->name = "Entity " + str(id++);
	e_prefab->add_child(e);
	return true;
}

bool App::cmd_delete_selected_entity()
{
	if (selection.type != Selection::tEntity)
		return false;
	auto e = selection.entity;
	if (e == e_prefab)
		return false;
	if (!e->prefab && get_prefab_instance(e))
	{
		show_message_dialog("[RestructurePrefabInstanceWarnning]");
		return false;
	}
	e->parent->remove_child(e);
	selection.clear();
	return true;
}

void App::show_message_dialog(const std::string& title, const std::string& content)
{
	open_message_dialog = true;
	message_dialog_title = title;
	message_dialog_text = content;
	if (title == "[RestructurePrefabInstanceWarnning]")
	{
		message_dialog_title = "Cannot restructure Prefab Instance";
		message_dialog_text = "You cannot add/remove/reorder entity or component in Prefab Instance\n"
			"Edit it in that prefab";
	}
}

PrefabInstance* get_prefab_instance(EntityPtr e)
{
	while (e)
	{
		if (e->prefab)
			return e->prefab.get();
		e = e->parent;
	}
	return nullptr;
}

int main(int argc, char** args)
{
	auto ap = parse_args(argc, args);

	app.init();

	std::filesystem::path preferences_path = L"preferences.ini";

	auto preferences_i = parse_ini_file(preferences_path);
	for (auto& e : preferences_i.get_section_entries("opened_windows"))
	{
		for (auto w : views)
		{
			if (w->name == e.value)
			{
				w->open();
				break;
			}
		}
	}
	for (auto& e : preferences_i.get_section_entries("project_path"))
	{
		app.open_project(e.value);
		break;
	}
	for (auto& e : preferences_i.get_section_entries("opened_prefab"))
	{
		app.open_prefab(e.value);
		break;
	}

	app.run();

	std::ofstream preferences_o(preferences_path);
	preferences_o << "[opened_windows]\n";
	for (auto w : views)
	{
		if (w->lis)
			preferences_o << w->name << "\n";
	}
	if (!app.project_path.empty())
	{
		preferences_o << "[project_path]\n";
		preferences_o << app.project_path.string() << "\n";
	}
	if (app.e_prefab)
	{
		preferences_o << "[opened_prefab]\n";
		preferences_o << app.prefab_path.string() << "\n";
	}
	preferences_o.close();

	return 0;
}
