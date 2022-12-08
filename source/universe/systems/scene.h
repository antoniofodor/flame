#pragma once

#include "../system.h"

namespace flame
{
	struct DrawData;

	// Reflect ctor
	struct sScene : System
	{
		OctNode* octree = nullptr;

		// Reflect
		virtual void generate_navmesh(float agent_radius, float agent_height, float walkable_climb, float walkable_slope_angle) = 0;
		// Reflect
		virtual std::vector<vec3> query_navmesh_path(const vec3& start, const vec3& end) = 0;
		// Reflect
		virtual bool raycast_navmesh(const vec3& start, const vec3& end, vec3& res) = 0;
		// Reflect
		virtual void get_debug_draw(DrawData& draw_data) = 0;

		struct Instance
		{
			virtual sScenePtr operator()() = 0;
		};
		FLAME_UNIVERSE_API static Instance& instance;

		struct Create
		{
			virtual sScenePtr operator()(WorldPtr) = 0;
		};
		// Reflect static
		FLAME_UNIVERSE_API static Create& create;
	};
}
