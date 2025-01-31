#pragma once

#include "scene.h"
#include "../components/node_private.h"

#ifdef USE_RECASTNAV
#include <DetourCrowd.h>
#include <DetourTileCache.h>
#include <Recast.h>
extern dtTileCache* dt_tile_cache;
extern dtNavMesh* dt_nav_mesh;
extern dtNavMeshQuery* dt_nav_query;
extern dtQueryFilter dt_filter;
extern dtCrowd* dt_crowd;

dtPolyRef dt_nearest_poly(const vec3& pos, const vec3& ext, vec3* pt = nullptr);
bool dt_init_nav_query();
bool dt_init_crowd();
#endif

namespace flame
{
	struct sScenePrivate : sScene
	{
	#ifdef USE_RECASTNAV
	#endif

		sScenePrivate();
		~sScenePrivate();

		void navmesh_generate(const std::vector<EntityPtr>& nodes, float agent_radius, float agent_height, float walkable_climb, float walkable_slope_angle) override;
		void navmesh_clear() override;
		bool navmesh_nearest_point(const vec3& center, const vec3& ext, vec3& res) override;
		std::vector<vec3> navmesh_query_path(const vec3& start, const vec3& end, uint max_smooth) override;
		bool navmesh_check_free_space(const vec3& pos, float radius) override;
		std::vector<vec3> navmesh_get_mesh() override;
		void navmesh_save(const std::filesystem::path& filename) override;
		void navmesh_load(const std::filesystem::path& filename) override;
		void draw_debug_primitives() override;

		void update() override;
	};
}
