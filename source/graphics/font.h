#pragma once

#include"graphics.h"

namespace flame
{
	namespace graphics
	{
		struct FontAtlas
		{
			FontAtlasType type;
			ImageViewPtr view;

			std::vector<std::filesystem::path> font_names;
			uint ref = 0;

			virtual ~FontAtlas() {}

			virtual const Glyph& get_glyph(wchar_t unicode, uint font_size) = 0;
			inline void init_latin_glyphs(uint font_size)
			{
				for (auto ch = 0x0020; ch <= 0x00FF; ch++)
					get_glyph(ch, font_size);
			}

			virtual float get_scale() = 0;

			FLAME_GRAPHICS_API static ivec2 icons_range();
			FLAME_GRAPHICS_API static wchar_t icon(uint hash);
			static inline std::string icon_s(uint hash)
			{
				return w2s(std::wstring(1, icon(hash)));
			}

			struct Get
			{
				virtual FontAtlasPtr operator()(const std::vector<std::filesystem::path>& font_names, FontAtlasType type = FontAtlasBitmap) = 0;
			};
			FLAME_GRAPHICS_API static Get& get;

			struct Release
			{
				virtual void operator()(FontAtlasPtr atlas) = 0;
			};
			FLAME_GRAPHICS_API static Release& release;
		};
	}
}
