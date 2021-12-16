#pragma once

#include "graphics.h"

namespace flame
{
	namespace graphics
	{
		struct DescriptorPool
		{
			virtual ~DescriptorPool() {}

			struct Current
			{
				virtual DescriptorPoolPtr operator()(DevicePtr device = nullptr) = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Current& current;

			struct Create
			{
				virtual DescriptorPoolPtr operator()(DevicePtr device) = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Create& create;
		};

		struct DescriptorBinding
		{
			DescriptorType type = DescriptorNone;
			uint count = 1;
			std::string name;

			UdtInfo* ui = nullptr;
		};

		struct DescriptorSetLayout
		{
			std::vector<DescriptorBinding> bindings;

			std::filesystem::path filename;

			virtual ~DescriptorSetLayout() {}

			inline int find_binding(std::string_view name) const
			{
				for (auto i = 0; i < bindings.size(); i++)
				{
					if (bindings[i].name == name)
						return i;
				}
				return -1;
			}

			struct Create
			{
				virtual DescriptorSetLayoutPtr operator()(DevicePtr device, std::span<DescriptorBinding> bindings) = 0;
				virtual DescriptorSetLayoutPtr operator()(DevicePtr device, const std::string& content, const std::string& key = "") = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Create& create;

			struct Get
			{
				virtual DescriptorSetLayoutPtr operator()(DevicePtr device, const std::filesystem::path& filename) = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Get& get;
		};

		struct DescriptorSet
		{
			DescriptorSetLayoutPtr layout;

			virtual ~DescriptorSet() {}

			virtual void set_buffer(uint binding, uint index, BufferPtr buf, uint offset = 0, uint range = 0) = 0;
			virtual void set_image(uint binding, uint index, ImageViewPtr iv, SamplerPtr sp) = 0;
			virtual void update() = 0;

			struct Create
			{
				virtual DescriptorSetPtr operator()(DescriptorPoolPtr pool, DescriptorSetLayoutPtr layout) = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Create& create;
		};

		struct PipelineLayout
		{
			std::vector<DescriptorSetLayoutPtr> descriptor_set_layouts;

			UdtInfo* pc_ui = nullptr;
			uint pc_sz = 0;

			std::filesystem::path filename;

			virtual ~PipelineLayout() {}

			struct Create
			{
				virtual PipelineLayoutPtr operator()(DevicePtr device, std::span<DescriptorSetLayoutPtr> descriptor_set_layouts, uint push_constant_size) = 0;
				virtual PipelineLayoutPtr operator()(DevicePtr device, const std::string& content, const std::string& key = "") = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Create& create;

			struct Get
			{
				virtual PipelineLayoutPtr operator()(DevicePtr device, const std::filesystem::path& filename) = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Get& get;
		};

		struct Shader
		{
			ShaderStageFlags type = ShaderStageNone;
			std::filesystem::path filename;
			std::vector<std::string> defines;

			UdtInfo* in_ui = nullptr;
			UdtInfo* out_ui = nullptr;

			virtual ~Shader() {}

			struct Create
			{
				virtual ShaderPtr operator()(DevicePtr device, ShaderStageFlags type, const std::string& content, const std::vector<std::string>& defines, const std::string& key = "") = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Create& create;

			struct Get
			{
				virtual ShaderPtr operator()(DevicePtr device, ShaderStageFlags type, const std::filesystem::path& filename, const std::vector<std::string>& defines) = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Get& get;
		};

		struct VertexAttributeInfo
		{
			int location = -1;
			int offset = -1;
			Format format;
		};

		struct VertexBufferInfo
		{
			std::vector<VertexAttributeInfo> attributes;
			VertexInputRate rate = VertexInputRateVertex;
			uint stride = 0;
		};

		/*
			if (Enable)
			{
				finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
				finalColor.a   = (srcAlphaBlendFactor * newColor.a  ) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a  );
			}
			else
				finalColor = newColor;

			finalColor = finalColor & colorWriteMask;
		*/
		struct BlendOption
		{
			bool enable = false;
			BlendFactor src_color = BlendFactorZero;
			BlendFactor dst_color = BlendFactorZero;
			BlendFactor src_alpha = BlendFactorZero;
			BlendFactor dst_alpha = BlendFactorZero;
		};

		struct GraphicsPipelineInfo
		{
			PipelineLayoutPtr layout = nullptr;
			std::vector<ShaderPtr> shaders;
			RenderpassPtr renderpass = nullptr;
			uint subpass_index = 0;
			std::vector<VertexBufferInfo> vertex_buffers;
			PrimitiveTopology primitive_topology = PrimitiveTopologyTriangleList;
			uint patch_control_points = 0;
			PolygonMode polygon_mode = PolygonModeFill;
			CullMode cull_mode = CullModeBack;
			SampleCount sample_count = SampleCount_1;
			bool alpha_to_coverage = false;
			bool depth_test = true;
			bool depth_write = true;
			CompareOp compare_op = CompareOpLess;
			std::vector<BlendOption> blend_options;
			std::vector<DynamicState> dynamic_states;
		};

		struct ComputePipelineInfo
		{
			ShaderPtr shader;
			PipelineLayoutPtr layout;
		};

		struct GraphicsPipeline
		{
			GraphicsPipelineInfo info;

			std::filesystem::path filename;
			std::vector<std::string> defines;

			virtual ~GraphicsPipeline() {}

			struct Create
			{
				virtual GraphicsPipelinePtr operator()(DevicePtr device, const GraphicsPipelineInfo& info) = 0;
				virtual GraphicsPipelinePtr operator()(DevicePtr device, const std::string& content, const std::vector<std::string>& defines, const std::string& key = "") = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Create& create;

			struct Get
			{
				virtual GraphicsPipelinePtr operator()(DevicePtr device, const std::filesystem::path& filename, const std::vector<std::string>& defines) = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Get& get;
		};

		struct ComputePipeline
		{
			ComputePipelineInfo info;

			std::filesystem::path filename;
			std::vector<std::string> defines;

			virtual ~ComputePipeline() {}

			struct Create
			{
				virtual ComputePipelinePtr operator()(DevicePtr device, const ComputePipelineInfo& info) = 0;
				virtual ComputePipelinePtr operator()(DevicePtr device, const std::string& content, const std::vector<std::string>& defines, const std::string& key = "") = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Create& create;

			struct Get
			{
				virtual ComputePipelinePtr operator()(DevicePtr device, const std::filesystem::path& filename, const std::vector<std::string>& defines) = 0;
			};
			FLAME_GRAPHICS_EXPORTS static Get& get;
		};
	}
}
