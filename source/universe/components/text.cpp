#include <flame/graphics/font.h>
#include <flame/graphics/canvas.h>
#include "../world_private.h"
#include "aligner_private.h"
#include "text_private.h"

namespace flame
{
	void cTextPrivate::set_text(const std::wstring& _text)
	{
		text = _text;
		mark_text_changed();
	}

	void cTextPrivate::set_size(uint s)
	{
		if (size == s)
			return;
		size = s;
		if (element)
			element->on_entity_message(MessageElementDrawingDirty);
		on_entity_message(MessageElementSizeDirty);
		Entity::report_data_changed(this, S<ch("size")>::v);
	}

	void cTextPrivate::set_color(const Vec4c& col)
	{
		if (color == col)
			return;
		color = col;
		if (element)
			element->on_entity_message(MessageElementDrawingDirty);
		on_entity_message(MessageElementSizeDirty);
		Entity::report_data_changed(this, S<ch("color")>::v);
	}

	void cTextPrivate::on_gain_element()
	{
		element->drawers.push_back(this);
	}

	void cTextPrivate::on_lost_element()
	{
		std::erase_if(element->drawers, [&](const auto& i) {
			return i == this;
		});
	}

	void cTextPrivate::on_gain_type_setting()
	{
		on_entity_message(MessageElementSizeDirty);
	}

	void cTextPrivate::on_gain_canvas()
	{
		atlas = canvas->get_resource(0)->get_font_atlas();
	}

	void cTextPrivate::on_lost_canvas()
	{
		atlas = nullptr;
	}

	void cTextPrivate::mark_text_changed()
	{
		if (element)
			element->on_entity_message(MessageElementDrawingDirty);
		on_entity_message(MessageElementSizeDirty);
		Entity::report_data_changed(this, S<ch("text")>::v);
	}

	void cTextPrivate::draw(graphics::Canvas* canvas)
	{
		canvas->add_text(0, text.c_str(), nullptr, size, color, element->points[4], Mat2f(element->transform));
	}

	Vec2f cTextPrivate::measure()
	{
		if (!atlas)
			return Vec2f(0.f);
		return Vec2f(atlas->text_size(size, text.c_str()));
	}

	void cTextPrivate::on_added()
	{
		on_entity_message(MessageElementSizeDirty);
	}

	void cTextPrivate::on_entity_message(Message msg)
	{
		switch (msg)
		{
		case MessageElementSizeDirty:
			if (type_setting && (auto_width || auto_height))
				type_setting->add_to_sizing_list(this, (EntityPrivate*)entity);
			break;
		}
	}

	cText* cText::create()
	{
		return f_new<cTextPrivate>();
	}
}
