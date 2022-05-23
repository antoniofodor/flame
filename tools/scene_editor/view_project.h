#pragma once

#include "app.h"

#include <flame/foundation/system.h>
#include <flame/graphics/explorer_abstract.h>

struct View_Project : View
{
	graphics::ExplorerAbstract explorer;

	void* ev_watcher = nullptr;
	std::mutex mtx_changed_paths;
	std::map<std::filesystem::path, FileChangeFlags> changed_paths;

	View_Project();
	void init() override;
	void reset(const std::filesystem::path& assets_path);

	void on_draw() override;
};

extern View_Project view_project;
