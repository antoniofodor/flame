#pragma once

#include "../entity_private.h"
#include <flame/universe/components/text.h>

namespace flame
{
	namespace graphics
	{
		struct FontAtlas;
		struct Canvas;
	}

	struct cElementPrivate;

	struct cTextBridge : cText
	{
		void set_text(const wchar_t* text) override;
	};

	struct cTextPrivate : cTextBridge
	{
		cElementPrivate* element = nullptr;
		graphics::Canvas* canvas = nullptr;

		bool auto_width = true;
		bool auto_height = true;

		std::wstring text;
		uint font_size = 14;
		cvec4 color = cvec4(0, 0, 0, 255);

		int res_id = -1;
		graphics::FontAtlas* atlas = nullptr;

		const wchar_t* get_text() const override { return text.c_str(); }
		uint get_text_length() const override { return text.size(); }
		void set_text(const std::wstring& text);

		uint get_font_size() const override { return font_size; }
		void set_font_size(uint s) override;

		cvec4 get_color() const override { return color; }
		void set_color(const cvec4& col) override;

		bool get_auto_width() const override { return auto_width; }
		void set_auto_width(bool a) override { auto_width = a; }
		bool get_auto_height() const override { return auto_height; }
		void set_auto_height(bool a) override { auto_height = a; }

		void on_gain_canvas();
		void on_lost_canvas();

		void mark_text_changed();

		void draw(graphics::Canvas* canvas);

		void measure(vec2& ret);
	};

	inline void cTextBridge::set_text(const wchar_t* text)
	{
		((cTextPrivate*)this)->set_text(text);
	}
}
