#pragma once

#include <flame/serialize.h>
#include <flame/universe/utils/ui.h>
#include <flame/utils/app.h>

using namespace flame;
using namespace graphics;

struct cEditor : Component
{
	cText* tab_text;
	Entity* e_base;

	BP::Slot* dragging_slot;

	cEditor();
	virtual ~cEditor() override;
	void on_deselect();
	void on_select();
	void on_changed();
	void on_load();
	void on_add_node(BP::Node* n);
	void on_remove_node(BP::Node* n);
	void on_data_changed(BP::Slot* s);
};

struct cConsole : Component
{
	cText* c_text_log;
	cEdit* c_edit_input;

	cConsole();
	virtual ~cConsole() override;
};

enum SelType
{
	SelAir,
	SelNode,
	SelLink
};

struct MyApp : App
{
	std::filesystem::path filepath;
	std::filesystem::path fileppath;
	BP* bp;
	bool changed;
	bool locked;
	bool auto_update;

	SelType sel_type;

	union
	{
		void* plain;
		BP::Node* node;
		BP::Slot* slot;
		BP::Slot* link;
	}selected;

	cEditor* editor;
	cConsole* console;

	MyApp()
	{
		editor = nullptr;
		console = nullptr;

		bp = nullptr;
		changed = false;
		locked = false;

		sel_type = SelAir;
		selected.plain = nullptr;
		auto_update = false;
	}

	void set_changed(bool v);

	void deselect();
	void select(SelType t, void* p);

	BP::Library* add_library(const wchar_t* filename);
	BP::Node* add_node(const char* type_name, const char* id, const Vec2f& pos);
	void remove_library(BP::Library* l);
	bool remove_node(BP::Node* n);

	void duplicate_selected();
	void delete_selected();

	void link_test_nodes();

	void update_gv();
	bool generate_graph_image();
	bool auto_set_layout();

	bool create(const char* filename);
	void load(const std::filesystem::path& filepath);
};

extern MyApp app;
