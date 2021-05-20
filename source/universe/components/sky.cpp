#include "../../graphics/device.h"
#include "../../graphics/image.h"
#include "../entity_private.h"
#include "../world_private.h"
#include "sky_private.h"
#include "../systems/renderer_private.h"

namespace flame
{
	void cSkyPrivate::set_box_texture(const char* name)
	{
		box_texture_name = name;
	}

	void cSkyPrivate::set_irr_texture(const char* name)
	{
		irr_texture_name = name;
	}

	void cSkyPrivate::set_rad_texture(const char* name)
	{
		rad_texture_name = name;
	}

	void cSkyPrivate::set_lut_texture(const char* name)
	{
		lut_texture_name = name;
	}

	void cSkyPrivate::on_entered_world()
	{
		s_renderer = entity->world->get_system_t<sRendererPrivate>();
		fassert(s_renderer);

		auto device = graphics::Device::get_default();
		auto ppath = entity->get_src(src_id).parent_path();

		{
			auto fn = std::filesystem::path(box_texture_name);
			if (!fn.extension().empty() && !fn.is_absolute())
				fn = ppath / fn;
			box_texture = graphics::Image::get(device, fn.c_str(), true);
		}
		{
			auto fn = std::filesystem::path(irr_texture_name);
			if (!fn.extension().empty() && !fn.is_absolute())
				fn = ppath / fn;
			irr_texture = graphics::Image::get(device, fn.c_str(), true);
		}
		{
			auto fn = std::filesystem::path(rad_texture_name);
			if (!fn.extension().empty() && !fn.is_absolute())
				fn = ppath / fn;
			rad_texture = graphics::Image::get(device, fn.c_str(), true);
		}
		{
			auto fn = std::filesystem::path(lut_texture_name);
			if (!fn.extension().empty() && !fn.is_absolute())
				fn = ppath / fn;
			lut_texture = graphics::Image::get(device, fn.c_str(), true);
		}

		s_renderer->set_sky(box_texture->get_view(box_texture->get_levels()), irr_texture->get_view(irr_texture->get_levels()),
			rad_texture->get_view(rad_texture->get_levels()), lut_texture->get_view(), this);
	}

	void cSkyPrivate::on_left_world()
	{
		graphics::ImageView* iv_box; 
		graphics::ImageView* iv_irr; 
		graphics::ImageView* iv_rad; 
		graphics::ImageView* iv_lut;
		void* id;
		s_renderer->get_sky(&iv_box, &iv_irr, &iv_rad, &iv_lut, &id);
		if (id == this)
			s_renderer->set_sky(nullptr, nullptr, nullptr, nullptr, nullptr);

		s_renderer = nullptr;
	}

	cSky* cSky::create(void* parms)
	{
		return f_new<cSkyPrivate>();
	}
}
