#include <flame/graphics/device.h>
#include <flame/graphics/font.h>
#include <flame/graphics/canvas.h>
#include "../world_private.h"
#include "element_private.h"
#include "text_private.h"
#include "../systems/renderer_private.h"

namespace flame
{
	void cTextPrivate::set_text(const std::wstring& _text)
	{
		text = _text;
		mark_text_changed();
	}

	void cTextPrivate::set_font_size(uint s)
	{
		if (font_size == s)
			return;
		font_size = s;
		if (element)
		{
			element->mark_drawing_dirty();
			element->mark_size_dirty();
		}
		if (entity)
			entity->component_data_changed(this, S<"font_size"_h>);
	}

	void cTextPrivate::set_font_color(const cvec4& col)
	{
		if (font_color == col)
			return;
		font_color = col;
		if (element)
		{
			element->mark_drawing_dirty();
			element->mark_size_dirty();
		}
		if (entity)
			entity->component_data_changed(this, S<"font_color"_h>);
	}

	void cTextPrivate::row_layout(int offset, vec2& size, uint& num_chars)
	{
		auto width = 0.f;

		auto beg = text.c_str() + offset;
		auto end = text.c_str() + text.size();
		auto s = beg;
		while (s < end)
		{
			auto c = *s++;
			if (c == L'\n')
				break;
			if (c == L'\r')
				continue;

			width += atlas->get_glyph(c, font_size).advance;
		}

		width = max(width, (float)font_size);

		size = vec2(width, font_size);
		num_chars = s - beg;
	}

	void cTextPrivate::mark_text_changed()
	{
		if (element)
		{
			element->mark_drawing_dirty();
			element->mark_size_dirty();
		}
		if (entity)
			entity->component_data_changed(this, S<"text"_h>);
	}

	uint cTextPrivate::draw(uint layer, sRenderer* renderer)
	{
		layer++;
		renderer->draw_text(layer, element, element->padding.xy(), font_size, res_id, text.c_str(), text.c_str() + text.size(), font_color);
		return layer;
	}

	void cTextPrivate::measure(vec2* ret)
	{
		if (!atlas)
		{
			*ret = vec2(-1.f);
			return;
		}
		*ret = vec2(atlas->text_size(font_size, text.c_str()));
	}

	void cTextPrivate::on_added()
	{
		element = entity->get_component_t<cElementPrivate>();
		fassert(element);

		drawer = element->add_drawer([](Capture& c, uint layer, sRenderer* renderer) {
			auto thiz = c.thiz<cTextPrivate>();
			return thiz->draw(layer, renderer);
		}, Capture().set_thiz(this));
		measurer = element->add_measurer([](Capture& c, vec2* ret) {
			auto thiz = c.thiz<cTextPrivate>();
			thiz->measure(ret);
		}, Capture().set_thiz(this));
		element->mark_drawing_dirty();
		element->mark_size_dirty();
	}

	void cTextPrivate::on_removed()
	{
		element->remove_drawer(drawer);
		element->remove_measurer(measurer);
		element->mark_drawing_dirty();
		element->mark_size_dirty();
	}

	void cTextPrivate::on_entered_world()
	{
		renderer = entity->world->get_system_t<sRenderer>();
		fassert(renderer);

		if (res_id != -1)
		{
			if (!atlas)
				atlas = (graphics::FontAtlas*)renderer->get_element_res(res_id, nullptr);
		}
		else
		{
			atlas = graphics::FontAtlas::get(graphics::Device::get_default(), L"msyh.ttc;font_awesome.ttf");
			res_id = renderer->find_element_res(atlas);
			if (res_id == -1)
				res_id = renderer->set_element_res(-1, sRenderer::ElementResFont, atlas);
			if (res_id == -1)
				atlas = nullptr;
		}
	}

	void cTextPrivate::on_left_world()
	{
		renderer = nullptr;
		res_id = -1;
		atlas = nullptr;
	}

	cText* cText::create(void* parms)
	{
		return f_new<cTextPrivate>();
	}
}
