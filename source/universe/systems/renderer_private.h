#pragma once

#include "../../graphics/command.h"
#include "renderer.h"

namespace flame
{
	enum MaterialUsage
	{
		MaterialForMesh,
		MaterialForMeshArmature,
		MaterialForMeshShadow,
		MaterialForMeshShadowArmature,
		MaterialMeshUsageCount,
		MaterialForTerrain = MaterialMeshUsageCount,

		MaterialUsageCount
	};

	inline uint const ArmatureMaxBones = 128;

	struct ElemnetRenderData;
	struct NodeRenderData;

	struct sRendererPrivate : sRenderer
	{
		bool always_update = false;

		graphics::Device* device;
		graphics::DescriptorPool* dsp;
		graphics::Sampler* sp_nearest;
		graphics::Sampler* sp_linear;

		graphics::Renderpass* rp_rgba8;
		graphics::Renderpass* rp_rgba8c;
		graphics::Renderpass* rp_bgra8;
		graphics::Renderpass* rp_bgra8c;
		std::vector<graphics::ImageView*> iv_tars;
		std::vector<UniPtr<graphics::Framebuffer>> fb_tars;
		uvec2 tar_sz;

		UniPtr<graphics::Image> img_white;
		UniPtr<graphics::Image> img_black;
		UniPtr<graphics::Image> img_dst;

		cCameraPrivate* camera = nullptr;

		RenderType render_type = RenderShaded;

		void*					sky_id = nullptr;

		std::unique_ptr<ElemnetRenderData>	_ed;
		std::unique_ptr<NodeRenderData>		_nd;

		bool dirty = true;

		sRendererPrivate(sRendererParms* parms);
		~sRendererPrivate();

		void set_always_update(bool a) override { always_update = a; }
		void set_render_type(RenderType type) override { render_type = type; }
		void set_shadow_props(uint dir_levels, float dir_dist, float pt_dist) override;

		graphics::ImageView* get_element_res(uint idx) const override;
		int set_element_res(int idx, graphics::ImageView* iv) override;
		int find_element_res(graphics::ImageView* iv) const override;

		void fill(uint layer, uint pt_cnt, const vec2* pts, const cvec4& color) override;
		void stroke(uint layer, uint pt_cnt, const vec2* pts, float thickness, const cvec4& color, bool closed) override;
		void draw_glyphs(uint layer, uint cnt, const graphics::GlyphDraw* glyphs, uint res_id, const cvec4& color) override;
		void draw_image(uint layer, const vec2* pts, uint res_id, const vec2& uv0, const vec2& uv1, const cvec4& tint_color) override;

		int set_texture_res(int idx, graphics::ImageView* tex, graphics::Sampler* sp) override;
		int find_texture_res(graphics::ImageView* tex) const override;

		int set_material_res(int idx, graphics::Material* mat) override;
		int find_material_res(graphics::Material* mat) const override;

		int set_mesh_res(int idx, graphics::Mesh* mesh) override;
		int find_mesh_res(graphics::Mesh* mesh) const override;

		graphics::Pipeline* get_material_pipeline(MaterialUsage usage, const std::filesystem::path& mat, const std::string& defines);
		void release_material_pipeline(MaterialUsage usage, graphics::Pipeline* pl);

		cCameraPtr get_camera() const override { return camera; }
		void set_camera(cCameraPtr c) override { camera = c; }

		void* get_sky_id() override { return sky_id; }
		void set_sky(graphics::ImageView* box, graphics::ImageView* irr,
			graphics::ImageView* rad, graphics::ImageView* lut, const vec3& fog_color, float intensity, void* id) override;

		void add_light(cNodePtr node, LightType type, const vec3& color, bool cast_shadow) override;
		uint add_armature(uint bones_count, const mat4* bones) override;
		void draw_mesh(cNodePtr node, uint mesh_id, bool cast_shadow, int armature_id, ShadingFlags flags) override;
		void draw_terrain(cNodePtr node, const vec3& extent, const uvec2& blocks, uint tess_levels, uint height_map_id, uint normal_map_id,
			uint material_id, ShadingFlags flags) override;

		bool is_dirty() const override { return always_update || dirty; }
		void mark_dirty() override { dirty = true; }

		uint element_render(uint layer, cElementPrivate* element);
		void node_render(cNodePrivate* node);

		void set_targets(uint tar_cnt, graphics::ImageView* const* ivs) override;
		void record(uint tar_idx, graphics::CommandBuffer* cb) override;

		void on_added() override;

		void update() override;
	};
}
