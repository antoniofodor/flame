// MIT License
// 
// Copyright (c) 2018 wjs
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <flame/graphics/graphics.h>

namespace flame
{
	namespace graphics
	{
		struct Device;
		struct Imageview;
		struct Swapchain;
		struct Commandbuffer;
		struct FontAtlas;

		enum DrawCmdType
		{
			DrawCmdElement,
			DrawCmdTextLcd,
			DrawCmdTextSdf,
			DrawCmdScissor
		};

		struct Canvas
		{
			FLAME_GRAPHICS_EXPORTS void set_clear_color(const Vec4c& col);

			FLAME_GRAPHICS_EXPORTS Imageview* get_imageview(int index);
			FLAME_GRAPHICS_EXPORTS void set_imageview(int index, Imageview* v);

			FLAME_GRAPHICS_EXPORTS int add_font_atlas(FontAtlas* font_atlas);
			FLAME_GRAPHICS_EXPORTS FontAtlas* get_font_atlas(int idx);

			FLAME_GRAPHICS_EXPORTS void start_cmd(DrawCmdType type, int id);
			FLAME_GRAPHICS_EXPORTS void path_line_to(const Vec2f& p);
			FLAME_GRAPHICS_EXPORTS void path_rect(const Vec2f& pos, const Vec2f& size, float round_radius, int round_flags);
			FLAME_GRAPHICS_EXPORTS void path_arc_to(const Vec2f& center, float radius, int a_min, int a_max);
			FLAME_GRAPHICS_EXPORTS void path_bezier(const Vec2f& p1, const Vec2f& p2, const Vec2f& p3, const Vec2f& p4, int level = 0);
			FLAME_GRAPHICS_EXPORTS void clear_path();
			FLAME_GRAPHICS_EXPORTS void stroke(const Vec4c& col, float thickness, bool closed);
			FLAME_GRAPHICS_EXPORTS void stroke_col2(const Vec4c& inner_col, const Vec4c& outter_col, float thickness, bool closed);
			FLAME_GRAPHICS_EXPORTS void fill(const Vec4c& col);

			FLAME_GRAPHICS_EXPORTS void add_text(int font_atlas_index, const Vec2f& pos, const Vec4c& col, const wchar_t* text, float scale = 1.f /* for sdf */);
			FLAME_GRAPHICS_EXPORTS void add_line(const Vec2f& p0, const Vec2f& p1, const Vec4c& col, float thickness);
			FLAME_GRAPHICS_EXPORTS void add_triangle_filled(const Vec2f& p0, const Vec2f& p1, const Vec2f& p2, const Vec4c& col);
			FLAME_GRAPHICS_EXPORTS void add_rect(const Vec2f& pos, const Vec2f& size, const Vec4c& col, float thickness, float round_radius = 0.f, int round_flags = SideNW | SideNE | SideSW | SideSE);
			FLAME_GRAPHICS_EXPORTS void add_rect_col2(const Vec2f& pos, const Vec2f& size, const Vec4c& inner_col, const Vec4c& outter_col, float thickness, float round_radius = 0.f, int round_flags = SideNW | SideNE | SideSW | SideSE);
			FLAME_GRAPHICS_EXPORTS void add_rect_rotate(const Vec2f& pos, const Vec2f& size, const Vec4c& col, float thickness, const Vec2f& rotate_center, float angle);
			FLAME_GRAPHICS_EXPORTS void add_rect_filled(const Vec2f& pos, const Vec2f& size, const Vec4c& col, float round_radius = 0.f, int round_flags = 0);
			FLAME_GRAPHICS_EXPORTS void add_circle(const Vec2f& center, float radius, const Vec4c& col, float thickness);
			void add_circle_LT(const Vec2f& center, float diameter, const Vec4c& col, float thickness)
			{
				add_circle(center + diameter * 0.5f, diameter * 0.5f, col, thickness);
			}
			FLAME_GRAPHICS_EXPORTS void add_circle_filled(const Vec2f& center, float radius, const Vec4c& col);
			void add_circle_filled_LT(const Vec2f& center, float diameter, const Vec4c& col)
			{
				add_circle_filled(center + diameter * 0.5f, diameter * 0.5f, col);
			}
			FLAME_GRAPHICS_EXPORTS void add_bezier(const Vec2f& p1, const Vec2f& p2, const Vec2f& p3, const Vec2f& p4, const Vec4c& col, float thickness);
			FLAME_GRAPHICS_EXPORTS void add_image(const Vec2f& pos, const Vec2f& size, int id, const Vec2f& uv0 = Vec2f(0.f), const Vec2f& uv1 = Vec2f(1.f), const Vec4c& tint_col = Vec4c(255));
			FLAME_GRAPHICS_EXPORTS void add_image_stretch(const Vec2f& pos, const Vec2f& size, int id, const Vec4f& border, const Vec4c& tint_col = Vec4c(255));
			FLAME_GRAPHICS_EXPORTS void set_scissor(const Vec4f& scissor);

			FLAME_GRAPHICS_EXPORTS Commandbuffer* get_cb() const;
			FLAME_GRAPHICS_EXPORTS void record_cb();

			FLAME_GRAPHICS_EXPORTS static void initialize(Device* d, Swapchain* sc);
			FLAME_GRAPHICS_EXPORTS static void deinitialize();

			FLAME_GRAPHICS_EXPORTS static Canvas* create(Swapchain* sc); // all swapchains that used to create canvas should have the same sample_count as the one pass to initialize
			FLAME_GRAPHICS_EXPORTS static void destroy(Canvas* c);
		};

		typedef Canvas* CanvasPtr;
	}
}
