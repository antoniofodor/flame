#pragma once

#include "canvas.h"
#include "buffer_private.h"
#include "shader_private.h"
#include "command_private.h"
#include "extension.h"

namespace flame
{
	namespace graphics
	{
		struct CanvasPrivate : Canvas
		{
			GraphicsPipelinePtr pl = nullptr;
			GraphicsPipelinePtr pl_stencil_write = nullptr;
			GraphicsPipelinePtr pl_stencil_compare = nullptr;
			GraphicsPipelinePtr pl_sdf = nullptr;
			GraphicsPipelinePtr pl_sdf_stencil_write = nullptr;
			GraphicsPipelinePtr pl_sdf_stencil_compare = nullptr;
			PipelineResourceManager prm;
			std::unique_ptr<Image> stencil_img;
			RenderpassPtr rp = nullptr;
			RenderpassPtr rp_load = nullptr;
			VertexBuffer buf_vtx;
			IndexBuffer<> buf_idx;
			ImagePtr main_img = nullptr;
			std::unique_ptr<DescriptorSetT> main_ds;

			CanvasPrivate(WindowPtr window);
			~CanvasPrivate();

			void create_rp(Format format);
			void set_targets(std::span<ImageViewPtr> targets) override;
			void bind_window_targets() override;
			void reset();
			DrawCmd& get_blit_cmd(DescriptorSetPtr ds);
			DrawCmd& get_sdf_cmd(DescriptorSetPtr ds, float sdf_scale, float thickness, float border);

			uint set_translate(const vec2& translate) override;
			void push_scissor(const Rect& rect) override;
			void pop_scissor() override;
			void begin_stencil_write() override;
			void end_stencil_write() override;
			void begin_stencil_compare() override;
			void end_stencil_compare() override;

			void path_rect(const vec2& a, const vec2& b);
			DrawVert* stroke_path(DrawCmd& cmd, float thickness, const cvec4& col, bool closed);
			DrawVert* fill_path(DrawCmd& cmd, const cvec4& col);
			DrawVert* stroke(float thickness, const cvec4& col, bool closed);
			DrawVert* fill(const cvec4& col);

			DrawVert*	add_rect(const vec2& a, const vec2& b, float thickness, const cvec4& col) override;
			DrawVert*	add_rect_filled(const vec2& a, const vec2& b, const cvec4& col) override;
			DrawVert*	add_circle(const vec2& p, float radius, float thickness, const cvec4& col) override;
			DrawVert*	add_circle_filled(const vec2& p, float radius, const cvec4& col) override;
			void		add_text(FontAtlasPtr font_atlas, uint font_size, const vec2& pos, std::wstring_view str, const cvec4& col, float thickness, float border) override;
			DrawVert*	add_image(ImageViewPtr view, const vec2& a, const vec2& b, const vec4& uvs, const cvec4& tint_col) override;
			DrawVert*	add_image_stretched(ImageViewPtr view, const vec2& a, const vec2& b, const vec4& uvs, const vec2& size, const vec4& border, const cvec4& tint_col) override;
			DrawVert*	add_image_rotated(ImageViewPtr view, const vec2& a, const vec2& b, const vec4& uvs, const cvec4& tint_col, float angle) override;

			void render(int idx, CommandBufferPtr cb);
		};
	}
}
