#pragma once

#include "node_renderer.h"

#include "../../graphics/extension.h"

namespace flame
{
	struct MeshRes
	{
		graphics::Mesh* mesh = nullptr;
		bool arm;
		uint vtx_off;
		uint vtx_cnt;
		uint idx_off;
		uint idx_cnt;
		std::vector<uint> mat_ids;
	};

	struct sNodeRendererPrivate : sNodeRenderer
	{
		std::vector<MeshRes> mesh_reses;

		graphics::RenderpassPtr rp_fwd;
		graphics::GraphicsPipelinePtr pl_mesh_fwd;
		std::vector<graphics::ImageViewPtr> iv_tars;
		std::vector<std::unique_ptr<graphics::Framebuffer>> fb_tars;

		std::unique_ptr<graphics::Image> img_dep;
		graphics::StorageBuffer<"flame::node_renderer::vtx"_h, graphics::BufferUsageVertex, false>			buf_vtx;
		graphics::StorageBuffer<"flame::node_renderer::idx"_h, graphics::BufferUsageIndex, false>			buf_idx;
		graphics::StorageBuffer<"flame::node_renderer::scene"_h, graphics::BufferUsageUniform, false>		buf_scene;
		graphics::StorageBuffer<"flame::node_renderer::mesh_transforms"_h, graphics::BufferUsageStorage>	buf_mesh_transforms;
		graphics::PipelineResourceManager<"flame::node_renderer::mesh_fwd"_h>								prm_mesh_fwd;

		graphics::ImageLayout dst_layout;

		bool initialized = false;

		sNodeRendererPrivate(graphics::WindowPtr w);

		void set_targets(std::span<graphics::ImageViewPtr> targets, graphics::ImageLayout dst_layout) override;

		int set_material_res(int idx, graphics::Material* mat) override;
		int find_material_res(graphics::Material* mat) const override;

		int set_mesh_res(int idx, graphics::Mesh* mesh) override;
		int find_mesh_res(graphics::Mesh* mesh) const override;

		uint add_mesh_transform(const mat4& mat, const mat3& nor) override;
		uint add_mesh_armature(const mat4* bones, uint count) override;
		void draw_mesh(uint id, uint mesh_id, uint skin, ShadingFlags flags) override;

		void update() override;
	};
}
