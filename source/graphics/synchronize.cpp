#include "device_private.h"
#include "synchronize_private.h"

namespace flame
{
	namespace graphics
	{
		SemaphorePrivate::SemaphorePrivate(DevicePrivate* d) :
			device(d)
		{
#if defined(FLAME_VULKAN)
			VkSemaphoreCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			chk_res(vkCreateSemaphore(device->vk_device, &info, nullptr, &vk_semaphore));
#endif
		}

		SemaphorePrivate::~SemaphorePrivate()
		{
#if defined(FLAME_VULKAN)
			vkDestroySemaphore(device->vk_device, vk_semaphore, nullptr);
#endif
		}

		Semaphore* Semaphore::create(Device* d)
		{
			return new SemaphorePrivate((DevicePrivate*)d);
		}

		FencePrivate::FencePrivate(DevicePrivate* d) :
			device(d)
		{
#if defined(FLAME_VULKAN)
			VkFenceCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			chk_res(vkCreateFence(device->vk_device, &info, nullptr, &vk_fence));
			value = 1;
#elif defined(FLAME_D3D12)
			auto res = d->v->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&v));
			ev = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			assert(ev);
			vl = 0;
#endif
		}

		FencePrivate::~FencePrivate()
		{
#if defined(FLAME_VULKAN)
			vkDestroyFence(device->vk_device, vk_fence, nullptr);
#elif defined(FLAME_D3D12)

#endif
		}

		void FencePrivate::wait()
		{
#if defined(FLAME_VULKAN)
			if (value > 0)
			{
				chk_res(vkWaitForFences(device->vk_device, 1, &vk_fence, true, UINT64_MAX));
				chk_res(vkResetFences(device->vk_device, 1, &vk_fence));
				value = 0;
			}
#elif defined(FLAME_D3D12)
			if (v->GetCompletedValue() < vl)
			{
				auto res = v->SetEventOnCompletion(vl, ev);
				assert(SUCCEEDED(res));

				WaitForSingleObject(ev, INFINITE);
			}
#endif
		}

		Fence* Fence::create(Device* d)
		{
			return new FencePrivate((DevicePrivate*)d);
		}
	}
}

