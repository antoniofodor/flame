#pragma once

#include "../component.h"

namespace flame
{
	// Reflect ctor
	struct cVolume : Component
	{
		// Reflect requires
		cNodePtr node = nullptr;

		// Reflect
		vec3 extent = vec3(256.f, 128.f, 256.f);
		// Reflect
		virtual void set_extent(const vec3& extent) = 0;

		// Reflect
		uvec3 blocks = uvec3(32);
		// Reflect
		virtual void set_blocks(const uvec3& blocks) = 0;

		// Reflect
		uint cells = 32;
		// Reflect
		virtual void set_cells(uint cells) = 0;

		// Reflect
		std::filesystem::path data_map_name;
		// Reflect
		virtual void set_data_map_name(const std::filesystem::path& name) = 0;

		// Reflect
		bool marching_cubes = true;

		graphics::ImagePtr data_map = nullptr;
		int instance_id = -1;

		struct Create
		{
			virtual cVolumePtr operator()(EntityPtr e) = 0;
		};
		// Reflect static
		FLAME_UNIVERSE_API static Create& create;
	};
}
