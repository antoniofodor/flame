#pragma once

#include <flame/serialize.h>
#include <flame/graphics/image.h>
#include <flame/utils/app.h>
#include <flame/utils/2d_editor.h>

using namespace flame;
using namespace graphics;

struct cEditor : Component
{
	utils::_2DEditor edt;

	int tool_type;

	cElement* gizmo;
	cElement* gizmo_target;
	void* gizmo_listener;

	cEditor();
	~cEditor() override;
	Entity* search_hovering(const Vec4f& r);
	void search_hovering_r(Entity* e, Entity*& s, const Vec4f& r);
	void update_gizmo();
	void on_select();
};

struct cResourceExplorer : Component
{
	std::filesystem::path base_path;
	std::filesystem::path curr_path;

	Entity* address_bar;
	Entity* e_list;
	cElement* c_list_element;
	cLayout* c_list_layout;
	Image* folder_img;
	Imageview* folder_img_v;
	uint folder_img_idx;
	Image* file_img;
	Imageview* file_img_v;
	uint file_img_idx;
	Image* thumbnails_img;
	Imageview* thumbnails_img_v;
	uint thumbnails_img_idx;
	Vec2u thumbnails_img_pos;
	std::vector<std::unique_ptr<Vec2u>> thumbnails_seats_free;
	std::vector<std::unique_ptr<Vec2u>> thumbnails_seats_occupied;

	std::filesystem::path selected;

	void* ev_file_changed;
	void* ev_end_file_watcher;

	cResourceExplorer();
	~cResourceExplorer() override;
	Entity* cResourceExplorer::create_listitem(const std::wstring& title, uint img_id);
	void cResourceExplorer::navigate(const std::filesystem::path& path);
	void cResourceExplorer::on_component_added(Component* c);
	void cResourceExplorer::draw(graphics::Canvas* canvas);
};

struct cHierarchy : Component
{
	Entity* e_tree;

	cHierarchy();
	~cHierarchy() override;
	Entity* find_item(Entity* e) const;
	void refresh_selected();
	void refresh();
};

struct cInspector : Component
{
	Entity* e_layout;

	cInspector();
	~cInspector() override;
	void refresh();
};

struct MyApp : App
{
	cEditor* editor;
	cResourceExplorer* resource_explorer;
	cHierarchy* hierarchy;
	cInspector* inspector;

	std::filesystem::path filepath;
	Entity* prefab;
	Entity* selected;

	MyApp()
	{
		editor = nullptr;
		resource_explorer = nullptr;
		hierarchy = nullptr;
		inspector = nullptr;

		prefab = nullptr;
		selected = nullptr;
	}

	void create();

	void select(Entity* e);
	void load(const std::filesystem::path& _filepath);
};

extern MyApp app;
