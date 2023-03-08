#pragma once

#include "app.h"

struct View_Scene : graphics::GuiView
{
	bool unsaved = false;

	std::unique_ptr<graphics::Image> render_tar;
	bool fixed_render_target_size = false;

	bool show_outline = true;
	bool show_AABB = false;
	bool show_axis = true;
	bool show_bones = false;
	bool show_navigation = false;

	uint camera_idx = 0;
	float camera_zoom = 5.f;
	cNodePtr hovering_node = nullptr;
	vec3 hovering_pos;

	View_Scene();

	cCameraPtr curr_camera();
	vec3 camera_target_pos();
	void focus_to_selected();
	void selected_to_focus();
	void on_draw() override;
	bool on_begin() override;
};

extern View_Scene view_scene;
