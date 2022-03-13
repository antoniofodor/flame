#pragma once

#include "terrain.h"
#include "node_private.h"

namespace flame
{
	struct cTerrainPrivate : cTerrain
	{
		int frame = -1;

		~cTerrainPrivate();
		void on_init() override;

		void set_extent(const vec3& extent) override;
		void set_blocks(const uvec2& blocks) override;
		void set_tess_level(uint tess_level) override;
		void set_height_map_name(const std::filesystem::path& name) override;
		void build_textures();

		void draw(sRendererPtr renderer);

		void on_active() override;
		void on_inactive() override;
	};
}
