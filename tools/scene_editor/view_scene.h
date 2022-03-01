#pragma once

#include "app.h"

struct View_Scene : View
{
	std::unique_ptr<graphics::Image> render_tar;

	bool show_AABB = false;

	float camera_zoom = 5.f;
	cNodePtr hovering_node = nullptr;
	vec3 hovering_pos;

	View_Scene();

	void on_draw() override;
};

extern View_Scene view_scene;
