#include "../../graphics/device.h"
#include "../../graphics/image.h"
#include "../../graphics/material.h"
#include "../entity_private.h"
#include "../world_private.h"
#include "node_private.h"
#include "terrain_private.h"
#include "../systems/renderer_private.h"

namespace flame
{
	void cTerrainPrivate::set_blocks(const uvec2& b)
	{
		if (blocks == b)
			return;
		blocks = b;
		if (node)
			node->mark_transform_dirty();
	}

	void cTerrainPrivate::set_tess_levels(uint l)
	{
		if (tess_levels == l)
			return;
		tess_levels = l;
		if (node)
			node->mark_transform_dirty();
	}

	void cTerrainPrivate::set_height_map(const std::string& name)
	{
		if (height_map_name == name)
			return;
		height_map_name = name;
	}

	void cTerrainPrivate::set_normal_map(const std::string& name)
	{
		if (normal_map_name == name)
			return;
		normal_map_name = name;
	}

	void cTerrainPrivate::set_material_name(const std::string& name)
	{
		if (material_name == name)
			return;
		material_name = name;
	}

	void cTerrainPrivate::draw(sRendererPrivate* s_renderer)
	{
		if (height_map_id != -1 && normal_map_id != -1 && material_id != -1)
			s_renderer->draw_terrain(node, blocks, tess_levels, height_map_id, normal_map_id, material_id);

		//auto flags = s_renderer->wireframe ? graphics::ShadeWireframe : graphics::ShadeMaterial;
		//if (entity->state & StateSelected)
		//	flags = flags | graphics::ShadeOutline;
		//canvas->draw_terrain(blocks, scale, node->g_pos, tess_levels, height_map_id, normal_map_id, material_id,
		//	flags, entity);
	}

	void cTerrainPrivate::on_added()
	{
		node = entity->get_component_i<cNodePrivate>(0);
		fassert(node);

		drawer = node->add_drawer([](Capture& c, sRendererPtr s_renderer) {
			auto thiz = c.thiz<cTerrainPrivate>();
			thiz->draw(s_renderer);
		}, Capture().set_thiz(this));
		node->mark_drawing_dirty();
	}

	void cTerrainPrivate::on_removed()
	{
		node->remove_drawer(drawer);
		node = nullptr;
	}

	void cTerrainPrivate::on_entered_world()
	{
		s_renderer = entity->world->get_system_t<sRendererPrivate>();
		fassert(s_renderer);

		auto device = graphics::Device::get_default();
		auto ppath = entity->get_src(src_id).parent_path();
		auto sp = graphics::Sampler::get(device, graphics::FilterLinear, graphics::FilterLinear, true, graphics::AddressClampToEdge);

		{
			auto fn = std::filesystem::path(height_map_name);
			if (!fn.extension().empty() && !fn.is_absolute())
				fn = ppath / fn;
			height_texture = graphics::Image::get(device, fn.c_str(), false);
			auto view = height_texture->get_view();
			height_map_id = s_renderer->find_texture_res(view);
			if (height_map_id == -1)
				height_map_id = s_renderer->set_texture_res(-1, view, sp);
		}
		{
			auto fn = std::filesystem::path(normal_map_name);
			if (!fn.extension().empty() && !fn.is_absolute())
				fn = ppath / fn;
			normal_texture = graphics::Image::get(device, fn.c_str(), false);
			auto view = normal_texture->get_view();
			normal_map_id = s_renderer->find_texture_res(view);
			if (normal_map_id == -1)
				normal_map_id = s_renderer->set_texture_res(-1, view, sp);
		}
		{
			auto fn = std::filesystem::path(material_name);
			if (!fn.extension().empty() && !fn.is_absolute())
				fn = ppath / fn;
			material = graphics::Material::get(fn.c_str());
			material_id = s_renderer->find_material_res(material);
			if (material_id == -1)
				material_id = s_renderer->set_material_res(-1, material);
		}
	}

	void cTerrainPrivate::on_left_world()
	{
		s_renderer = nullptr;
	}

	cTerrain* cTerrain::create(void* parms)
	{
		return f_new<cTerrainPrivate>();
	}
}
