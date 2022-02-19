#pragma once

#include "../system.h"

namespace flame
{
	/// Reflect
	struct sScene : System
	{
		OctNode* octree = nullptr;

		virtual void generate_navmesh(const std::filesystem::path& output) = 0;
		virtual std::vector<vec3> navmesh_calc_path(const vec3& start, const vec3& end) = 0;

		struct Instance
		{
			virtual sScenePtr operator()() = 0;
		};
		FLAME_UNIVERSE_API static Instance& instance;

		struct Create
		{
			virtual sScenePtr operator()(WorldPtr) = 0;
		};
		/// Reflect static
		FLAME_UNIVERSE_API static Create& create;
	};
}
