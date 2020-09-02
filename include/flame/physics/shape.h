#pragma once

#include <flame/physics/physics.h>

namespace flame
{
	namespace physics
	{
		struct Device;
		struct Rigid;
		struct Material;

		struct Shape
		{
			virtual void release() = 0;

			// trigger means it will not collide with others but will report when it overlay with others, default is false
			virtual void set_trigger(bool v) = 0;

			virtual Rigid* get_rigid() const = 0;

			FLAME_PHYSICS_EXPORTS static Shape* create(Device* d, Material* m, ShapeType type, const ShapeDesc& desc, const Vec3f& coord);
		};
	}
}

