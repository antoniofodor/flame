#pragma once

#include "volume.h"

namespace flame
{
	struct cVolumePrivate : cVolume
	{
		bool dirty = true;

		void set_extent(const vec3& extent) override;
		void set_blocks(const uvec3& blocks) override;
		void set_cells(uint cells) override;
		void set_data_map_name(const std::filesystem::path& name) override;

		~cVolumePrivate();
		void on_init() override;

		void on_active() override;
		void on_inactive() override;
	};
}
