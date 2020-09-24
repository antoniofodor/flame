#pragma once

#include <flame/graphics/graphics.h>

namespace flame
{
	namespace graphics
	{
		struct Material
		{

		};

		struct Mesh
		{
			virtual uint get_vertices_count() const = 0;
			virtual const Vec3f* get_positions() const = 0;
			virtual const Vec2f* get_uvs() const = 0;
			virtual const Vec3f* get_normals() const = 0;

			virtual uint get_indices_count() const = 0;
			virtual const uint* get_indices() const = 0;
		};
		
		struct Node
		{
			virtual const char* get_name() const = 0;

			virtual void get_transform(Vec3f& pos, Vec4f& quat, Vec3f& scale) const = 0;

			virtual Node* get_parent() const = 0;
			virtual uint get_children_count() const = 0;
			virtual Node* get_child(uint idx) const = 0;

			virtual int get_mesh_index() const = 0;
		};

		struct Model
		{
			virtual uint get_meshes_count() const = 0;
			virtual Mesh* get_mesh(uint idx) const = 0;

			virtual Node* get_root() const = 0;

			// name - which material you want to substitute
			// filename - .fmtl file
			virtual void substitute_material(const char* name, const wchar_t* filename) = 0;

			virtual void save(const wchar_t* filename, const char* model_name = nullptr) const = 0;

			FLAME_GRAPHICS_EXPORTS static Model* get_standard(const char* name);
			FLAME_GRAPHICS_EXPORTS static Model* create(const wchar_t* filename);
		};
	}
}
