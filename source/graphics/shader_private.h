#pragma once

#include "../foundation/typeinfo.h"
#include "shader.h"
#include "graphics_private.h"

namespace flame
{
	namespace graphics
	{
		struct DescriptorPoolPrivate : DescriptorPool
		{
			DevicePrivate* device;
			VkDescriptorPool vk_descriptor_pool;

			~DescriptorPoolPrivate();
		};

		struct DescriptorSetLayoutPrivate : DescriptorSetLayout
		{
			DevicePrivate* device;
			TypeInfoDataBase db;
			VkDescriptorSetLayout vk_descriptor_set_layout;

			uint ref = 0;

			~DescriptorSetLayoutPrivate();

			static DescriptorSetLayoutPtr load_from_res(DevicePtr device, const std::filesystem::path& filename);
		};

		struct DescriptorSetPrivate : DescriptorSet
		{
			struct BufRes
			{
				BufferPrivate* p;
				uint offset;
				uint range;
			};

			struct ImgRes
			{
				ImageViewPrivate* p;
				SamplerPrivate* sp;
			};

			union Res
			{
				BufRes b;
				ImgRes i;
			};

			DevicePrivate* device;
			DescriptorPoolPrivate* pool;
			VkDescriptorSet vk_descriptor_set;

			std::vector<std::vector<Res>> reses;
			std::vector<std::pair<uint, uint>> buf_updates;
			std::vector<std::pair<uint, uint>> img_updates;

			~DescriptorSetPrivate();

			void set_buffer(uint binding, uint index, BufferPtr buf, uint offset = 0, uint range = 0) override;
			void set_image(uint binding, uint index, ImageViewPtr iv, SamplerPtr sp) override;
			void update() override;
		};

		struct PipelineLayoutPrivate : PipelineLayout
		{
			DevicePrivate* device;
			TypeInfoDataBase db;
			VkPipelineLayout vk_pipeline_layout;

			uint ref = 0;

			~PipelineLayoutPrivate();

			static PipelineLayoutPtr load_from_res(DevicePtr device, const std::filesystem::path& filename);
		};

		struct ShaderPrivate : Shader
		{
			DevicePrivate* device;
			TypeInfoDataBase db;
			VkShaderModule vk_module = 0;

			uint ref = 0;

			~ShaderPrivate();

			static ShaderPtr load_from_res(DevicePtr device, const std::filesystem::path& filename);
		};

		struct GraphicsPipelinePrivate : GraphicsPipeline
		{
			DevicePrivate* device;
			VkPipeline vk_pipeline;

			uint ref = 0;

			~GraphicsPipelinePrivate();

			static GraphicsPipelinePtr load(DevicePtr device, const std::filesystem::path& filename, const std::vector<std::string>& defines);
		};

		struct ComputePipelinePrivate : ComputePipeline
		{
			DevicePrivate* device;
			VkPipeline vk_pipeline;

			uint ref = 0;

			~ComputePipelinePrivate();
		};
	}
}
