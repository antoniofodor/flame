#include "buffer.h"
#include "graphics_private.h"

namespace flame
{
	namespace graphics
	{
		struct BufferPrivate : Buffer
		{
			DevicePrivate* device;

			VkBuffer vk_buffer;
			VkDeviceMemory vk_memory;

			~BufferPrivate();

			void create();
			void destroy();

			void map(uint offset = 0, uint size = 0) override;
			void unmap() override;
			void flush() override;

			void recreate(uint new_size) override;
		};
	}
}

