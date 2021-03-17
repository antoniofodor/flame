#pragma once

#include <flame/universe/components/text.h>

namespace flame
{
	namespace graphics
	{
		struct FontAtlas;
		struct Canvas;
	}

	struct sRenderer;

	struct cElementPrivate;

	struct cTextBridge : cText
	{
		void set_text(const wchar_t* text) override;
	};

	struct cTextPrivate : cTextBridge
	{
		std::wstring text;
		uint font_size = 16;
		cvec4 font_color = cvec4(0, 0, 0, 255);

		cElementPrivate* element = nullptr;
		void* drawer = nullptr;
		void* measurer = nullptr;
		sRenderer* renderer = nullptr;

		int res_id = -1;
		graphics::FontAtlas* atlas = nullptr;

		const wchar_t* get_text() const override { return text.c_str(); }
		uint get_text_length() const override { return text.size(); }
		void set_text(const std::wstring& text);

		uint get_font_size() const override { return font_size; }
		void set_font_size(uint s) override;

		cvec4 get_font_color() const override { return font_color; }
		void set_font_color(const cvec4& col) override;

		void row_layout(int offset, vec2& size, uint& num_chars);

		void mark_text_changed();

		void draw(graphics::Canvas* canvas);
		uint draw2(uint layer, sRenderer* renderer);

		void measure(vec2* ret);

		void on_added() override;
		void on_removed() override;
		void on_entered_world() override;
		void on_left_world() override;
	};

	inline void cTextBridge::set_text(const wchar_t* text)
	{
		((cTextPrivate*)this)->set_text(text);
	}
}
