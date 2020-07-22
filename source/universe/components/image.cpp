#include <flame/graphics/image.h>
#include <flame/graphics/canvas.h>

#include "../world_private.h"
#include "../res_map_private.h"
#include "image_private.h"

namespace flame
{
	void cImagePrivate::set_res_id(uint id)
	{
		if (res_id == id)
			return;
		res_id = id;
		src = "";
	}

	void cImagePrivate::set_tile_id(uint id)
	{
		if (tile_id == id)
			return;
		tile_id = id;
		src = "";
	}

	void cImagePrivate::set_src(const std::string& _src)
	{
		if (src == _src)
			return;
		res_id = 0xffffffff;
		tile_id = 0;
		src = _src;
		if (canvas && res_map)
		{
			auto path = res_map->get_res_path(src);
			auto slot = 0;
			while (true)
			{
				auto r = canvas->get_resource(slot);
				if (!r)
					break;
				auto r_filename = std::filesystem::path(r->get_filename());
				if (r_filename == path)
				{
					res_id = slot;
					break;
				}
				slot++;
			}
		}
	}

	void cImagePrivate::on_added()
	{
		element = (cElementPrivate*)((EntityPrivate*)entity)->get_component(cElement::type_hash);
		element->drawers.push_back(this);
		if (type_setting && true/*auto_size*/)
			type_setting->add_to_sizing_list(this, (EntityPrivate*)entity);
	}

	void cImagePrivate::on_removed()
	{
		std::erase_if(element->drawers, [&](const auto& i) {
			return i == this;
		});
	}

	void cImagePrivate::on_entered_world()
	{
		auto world = ((EntityPrivate*)entity)->world;
		type_setting = (sTypeSettingPrivate*)world->get_system(sTypeSetting::type_hash);
		//if (auto_size)
			type_setting->add_to_sizing_list(this, (EntityPrivate*)entity);
		canvas = (graphics::Canvas*)world->find_object("Canvas");
		res_map = (ResMapPrivate*)world->find_object("ResMap");
		if (canvas && res_map && !src.empty())
		{
			auto _src = src;
			src = "";
			set_src(_src);
		}
	}

	void cImagePrivate::on_left_world()
	{
		if (type_setting)
			type_setting->remove_from_sizing_list(this);
		type_setting = nullptr;
		canvas = nullptr;
		res_map = nullptr;
	}

	void cImagePrivate::on_entity_visibility_changed()
	{
		if (type_setting && true/*auto_size*/)
			type_setting->add_to_sizing_list(this, (EntityPrivate*)entity);
	}

	Vec2f cImagePrivate::measure()
	{
		if (!canvas || res_id == 0xffffffff)
			return Vec2f(0.f);
		auto r = canvas->get_resource(res_id);
		auto ia = r->get_image_atlas();
		if (!ia)
			return Vec2f(r->get_view()->get_image()->get_size());
		return Vec2f(ia->get_tile(tile_id)->get_size());
	}

	void cImagePrivate::draw(graphics::Canvas* canvas)
	{
		canvas->add_image(res_id, tile_id, element->p00, Vec2f(element->width, element->height), uv0, uv1, Vec4c(255));
	}

	cImagePrivate* cImagePrivate::create()
	{
		return f_new<cImagePrivate>();
	}

	cImage* cImage::create() { return cImagePrivate::create(); }
}
