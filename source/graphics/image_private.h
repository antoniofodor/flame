#pragma once

#include <flame/graphics/image.h>
#include "graphics_private.h"

namespace flame
{
	namespace graphics
	{
		struct DevicePrivate;
		struct ImageviewPrivate;

		struct ImagePrivate : Image
		{
			DevicePrivate* d;

#if defined(FLAME_VULKAN)
			VkDeviceMemory m;
			VkImage v;
#elif defined(FLAME_D3D12)
			ID3D12Resource* v;
#endif
			ImageviewPrivate* dv;

			ImagePrivate(Device* d, Format format, const Vec2u& size, uint level, uint layer, SampleCount sample_count, ImageUsageFlags usage);
			ImagePrivate(Device* d, Format format, const Vec2u& size, uint level, uint layer, void* native);
			~ImagePrivate();

			void set_props();

			void clear(ImageLayout current_layout, ImageLayout after_layout, const Vec4c& color);
			void get_pixels(const Vec2u& offset, const Vec2u& extent, void* dst);
			void set_pixels(const Vec2u& offset, const Vec2u& extent, const void* src);
		};

		struct ImageviewPrivate : Imageview
		{
			ImagePrivate* image;
			DevicePrivate* d;
#if defined(FLAME_VULKAN)
			VkImageView v;
#elif defined(FLAME_D3D12)
			ID3D12DescriptorHeap* v;
#endif
			int ref_count;

			ImageviewPrivate(Image* image, ImageviewType type, uint base_level, uint level_count, uint base_layer, uint layer_count, Swizzle swizzle_r, Swizzle swizzle_g, Swizzle swizzle_b, Swizzle swizzle_a);
			~ImageviewPrivate();
		};

		inline ImageAspectFlags aspect_from_format(Format fmt)
		{
			if (fmt >= Format_Color_Begin && fmt <= Format_Color_End)
				return ImageAspectColor;
			if (fmt >= Format_Depth_Begin && fmt <= Format_Depth_End)
			{
				int a = ImageAspectDepth;
				if (fmt >= Format_DepthStencil_Begin && fmt <= Format_DepthStencil_End)
					a |= ImageAspectStencil;
				return a;
			}
			return ImageAspectColor;
		}

		struct SamplerPrivate : Sampler
		{
			DevicePrivate* d;
#if defined(FLAME_VULKAN)
			VkSampler v;
#elif defined(FLAME_D3D12)

#endif

			SamplerPrivate(Device* d, Filter mag_filter, Filter min_filter, bool unnormalized_coordinates);
			~SamplerPrivate();
		};

		struct AtlasTilePrivate : Atlas::Tile
		{
			std::wstring _filename;
		};

		struct AtlasPrivate : Atlas
		{
			Image* image;
			Imageview* imageview;
			std::vector<std::unique_ptr<AtlasTilePrivate>> tiles;

			AtlasPrivate(Device* d, const std::wstring& atlas_filename);
			~AtlasPrivate();
		};
	}
}

