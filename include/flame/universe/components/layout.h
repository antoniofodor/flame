#pragma once

#include <flame/universe/component.h>

namespace flame
{
	struct FLAME_RU(cLayout : Component, all)
	{
		inline static auto type_name = "cLayout";
		inline static auto type_hash = ch(type_name);

		cLayout() :
			Component(type_name, type_hash, true, true, true, true)
		{
		}

		virtual LayoutType get_type() const = 0;
		virtual void set_type(LayoutType t) = 0;

		virtual float get_gap() const = 0;
		virtual void set_gap(float g) = 0;

		virtual bool get_auto_width() const = 0;
		virtual void set_auto_width(bool a) = 0;
		virtual bool get_auto_height() const = 0;
		virtual void set_auto_height(bool a) = 0;

		//FLAME_RV(uint, column);

		//Vec2f content_size;

		virtual void mark_layout_dirty() = 0;

		//FLAME_UNIVERSE_EXPORTS void set_scrollx(float x);
		//FLAME_UNIVERSE_EXPORTS void set_scrolly(float y);
		//FLAME_UNIVERSE_EXPORTS void set_column(uint c);

		FLAME_UNIVERSE_EXPORTS static cLayout* create();
	};
}
