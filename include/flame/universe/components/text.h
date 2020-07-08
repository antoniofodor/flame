#pragma once

#include <flame/universe/component.h>

namespace flame
{
	struct FLAME_R(cText : Component, all)
	{
		inline static auto type_name = "cText";
		inline static auto type_hash = ch(type_name);

		cText() :
			Component(type_name, type_hash)
		{
		}

		virtual const wchar_t* get_string() const = 0;
		virtual void set_string(const wchar_t* str) = 0;

		//FLAME_RV(graphics::FontAtlas*, font_atlas);
		//FLAME_RV(uint, font_size);
		//FLAME_RV(Vec4c, color);
		//FLAME_RV(bool, auto_size);

		//FLAME_UNIVERSE_EXPORTS void FLAME_RF(set_text)(const wchar_t* text/* nullptr means no check (you need to set by yourself) */, int length = -1);
		//FLAME_UNIVERSE_EXPORTS void FLAME_RF(set_font_size)(uint s);
		//FLAME_UNIVERSE_EXPORTS void FLAME_RF(set_color)(const Vec4c& c);
		//FLAME_UNIVERSE_EXPORTS void FLAME_RF(set_auto_size)(bool v);

		FLAME_UNIVERSE_EXPORTS static cText* create();
	};
}
