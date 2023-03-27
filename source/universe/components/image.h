#pragma once

#include "../component.h"

namespace flame
{
	// Reflect ctor
	struct cImage : Component
	{
		// Reflect requires
		cElementPtr element = nullptr;

		// Reflect
		std::filesystem::path image_name;
		// Reflect
		virtual void set_image_name(const std::filesystem::path& image_name) = 0;

		graphics::ImagePtr image = nullptr;

		struct Create
		{
			virtual cImagePtr operator()(EntityPtr) = 0;
		};
		// Reflect static
		FLAME_UNIVERSE_API static Create& create;
	};
}
