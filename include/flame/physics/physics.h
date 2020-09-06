#pragma once

#ifdef FLAME_PHYSICS_MODULE
#define FLAME_PHYSICS_EXPORTS __declspec(dllexport)
#else
#define FLAME_PHYSICS_EXPORTS __declspec(dllimport)
#endif

#include <flame/math.h>

namespace flame
{
	namespace graphics
	{
		struct Model;
	}

	namespace physics
	{
		enum TouchType
		{
			TouchFound,
			TouchLost
		};

		enum ShapeType
		{
			ShapeBox,
			ShapeSphere,
			ShapeCapsule,
			ShapeTriangles
		};

		union ShapeDesc
		{
			struct
			{
				Vec3f hf_ext;
			}box;
			struct
			{
				float radius;
			}sphere;
			struct
			{
				float radius;
				float height;
			}capsule;
			struct
			{
				graphics::Model* model;
				uint mesh_idx;
				Vec3f scale;
			}triangles;
		};
	}
}

