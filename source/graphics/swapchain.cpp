#include <flame/foundation/blueprint.h>
#include "device_private.h"
#include "synchronize_private.h"
#include <flame/graphics/image.h>
#include <flame/graphics/renderpass.h>
#include "commandbuffer_private.h"
#include "swapchain_private.h"

#ifdef FLAME_ANDROID
#include <android_native_app_glue.h>
#endif

#include <flame/reflect_macros.h>

namespace flame
{
	namespace graphics
	{
		static auto swapchain_format = Format_Swapchain_B8G8R8A8_UNORM;

		Format get_swapchain_format()
		{
			return swapchain_format;
		}

		SwapchainPrivate::SwapchainPrivate(Device* _d, SysWindow* w) :
			d((DevicePrivate*)_d),
			w(w)
		{
			image_index = 0;

			auto size = w->size;
			uint image_count = 3;

#if defined(FLAME_VULKAN)

#ifdef FLAME_WINDOWS
			VkWin32SurfaceCreateInfoKHR surface_info;
			surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surface_info.flags = 0;
			surface_info.pNext = nullptr;
			surface_info.hinstance = (HINSTANCE)get_hinst();
			surface_info.hwnd = (HWND)w->get_native();
			chk_res(vkCreateWin32SurfaceKHR(d->instance, &surface_info, nullptr, &s));
#else
			VkAndroidSurfaceCreateInfoKHR surface_info;
			surface_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
			surface_info.flags = 0;
			surface_info.pNext = nullptr;
			surface_info.window = ((android_app*)w->get_native())->window;
#endif

			VkBool32 surface_supported;
			vkGetPhysicalDeviceSurfaceSupportKHR(d->physical_device, 0, s, &surface_supported);
			assert(surface_supported);

			unsigned int present_mode_count = 0;
			std::vector<VkPresentModeKHR> present_modes;
			vkGetPhysicalDeviceSurfacePresentModesKHR(d->physical_device, s, &present_mode_count, nullptr);
			present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(d->physical_device, s, &present_mode_count, present_modes.data());

			VkSurfaceCapabilitiesKHR surface_capabilities;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d->physical_device, s, &surface_capabilities);

			size.x() = clamp(size.x(),
				surface_capabilities.minImageExtent.width,
				surface_capabilities.maxImageExtent.width);
			size.y() = clamp(size.y(),
				surface_capabilities.minImageExtent.height,
				surface_capabilities.maxImageExtent.height);

			unsigned int surface_format_count = 0;
			std::vector<VkSurfaceFormatKHR> surface_formats;
			vkGetPhysicalDeviceSurfaceFormatsKHR(d->physical_device, s, &surface_format_count, nullptr);
			assert(surface_format_count > 0);
			surface_formats.resize(surface_format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(d->physical_device, s, &surface_format_count, surface_formats.data());

			swapchain_format = get_format(surface_formats[0].format, true);

			VkSwapchainCreateInfoKHR swapchain_info;
			swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchain_info.flags = 0;
			swapchain_info.pNext = nullptr;
			swapchain_info.surface = s;
			swapchain_info.minImageCount = image_count;
			swapchain_info.imageFormat = to_backend(swapchain_format);
			swapchain_info.imageColorSpace = surface_formats[0].colorSpace;
			swapchain_info.imageExtent.width = size.x();
			swapchain_info.imageExtent.height = size.y();
			swapchain_info.imageArrayLayers = 1;
			swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchain_info.queueFamilyIndexCount = 0;
			swapchain_info.pQueueFamilyIndices = nullptr;
			swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			swapchain_info.clipped = true;
			swapchain_info.oldSwapchain = 0;
			chk_res(vkCreateSwapchainKHR(d->v, &swapchain_info, nullptr, &v));

			std::vector<VkImage> native_images;
			vkGetSwapchainImagesKHR(d->v, v, &image_count, nullptr);
			native_images.resize(image_count);
			vkGetSwapchainImagesKHR(d->v, v, &image_count, native_images.data());

#elif defined(FLAME_D3D12)

			HRESULT res;

			DXGI_MODE_DESC backbuffer_desc = {};
			backbuffer_desc.Width = size.x();
			backbuffer_desc.Height = size.y();
			backbuffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

			DXGI_SAMPLE_DESC sample_desc = {};
			sample_desc.Count = 1;

			DXGI_SWAP_CHAIN_DESC desc = {};
			desc.BufferCount = image_count;
			desc.BufferDesc = backbuffer_desc;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			desc.OutputWindow = (HWND)(w->get_native());
			desc.SampleDesc = sample_desc;
			desc.Windowed = true;

			IDXGISwapChain* temp_swapchain;
			res = d->factory->CreateSwapChain(((QueuePrivate*)d->gq)->v, &desc, &temp_swapchain);
			assert(SUCCEEDED(res));
			v = (IDXGISwapChain3*)temp_swapchain;

			std::vector<void*> vk_images;

			std::vector<ID3D12Resource*> native_images;
			native_images.resize(image_count);
			for (auto i = 0; i < image_count; i++)
			{
				res = v->GetBuffer(i, IID_PPV_ARGS(&native_images[i]));
				assert(SUCCEEDED(res));
			}
#endif

			images.resize(image_count);
			for (auto i = 0; i < image_count; i++)
				images[i] = Image::create_from_native(d, swapchain_format, size, 1, 1, native_images[i]);

			image_avalible = (SemaphorePrivate*)Semaphore::create(d);
		}

		SwapchainPrivate::~SwapchainPrivate()
		{
			for (auto i : images)
				Image::destroy(i);

#if defined(FLAME_VULKAN)
			vkDestroySwapchainKHR(d->v, v, nullptr);
			vkDestroySurfaceKHR(d->instance, s, nullptr);
#elif defined(FLAME_D3D12)

#endif

			Semaphore::destroy(image_avalible);
		}

		void SwapchainPrivate::acquire_image()
		{
#if defined(FLAME_VULKAN)
			chk_res(vkAcquireNextImageKHR(d->v, v, UINT64_MAX, image_avalible->v, VK_NULL_HANDLE, &image_index));
#elif defined(FLAME_D3D12)
			image_index = v->GetCurrentBackBufferIndex();
#endif
		}

		SysWindow* Swapchain::window() const
		{
			return ((SwapchainPrivate*)this)->w;
		}

		uint Swapchain::image_count() const
		{
			return ((SwapchainPrivate*)this)->images.size();
		}

		Image* Swapchain::image(uint idx) const
		{
			return ((SwapchainPrivate*)this)->images[idx];
		}

		Semaphore* Swapchain::image_avalible() const
		{
			return ((SwapchainPrivate*)this)->image_avalible;
		}

		uint Swapchain::image_index() const
		{
			return ((SwapchainPrivate*)this)->image_index;
		}

		void Swapchain::acquire_image()
		{
			((SwapchainPrivate*)this)->acquire_image();
		}

		Swapchain *Swapchain::create(Device *d, SysWindow*w)
		{
			return new SwapchainPrivate(d, w);
		}

		void Swapchain::destroy(Swapchain *s)
		{
			delete (SwapchainPrivate*)s;
		}

		SwapchainResizablePrivate::SwapchainResizablePrivate(Device* d, SysWindow* w) :
			d(d),
			w(w)
		{
			sc = Swapchain::create(d, w);
			changed = true;

			auto thiz = this;
			resize_listener = w->resize_listeners.add([](void* c, const Vec2u& size) {
				auto thiz = *(SwapchainResizablePrivate**)c;
				if (thiz->sc)
				{
					thiz->d->gq->wait_idle();
					Swapchain::destroy(thiz->sc);
				}
				if (size.x() != 0 && size.y() != 0)
					thiz->sc = Swapchain::create(thiz->d, thiz->w);
				else
					thiz->sc = nullptr;
				thiz->changed = true;
				return true;
			}, new_mail(&thiz));
		}
		
		SwapchainResizablePrivate::~SwapchainResizablePrivate()
		{
			w->resize_listeners.remove(resize_listener);
			if (sc)
				Swapchain::destroy(sc);
		}

		Swapchain* SwapchainResizable::sc() const
		{
			return ((SwapchainResizablePrivate*)this)->sc;
		}

		void SwapchainResizable::link_bp(BP* bp, void* cbs)
		{
			bool ok;

			auto n_scr = bp->add_node("D#flame::graphics::R_SwapchainResizable", "scr");
			n_scr->find_input("in")->set_data_p(this);
			bp->find_input("*.rt_dst.type")->set_data_i(TargetImages);
			ok = bp->find_input("*.rt_dst.v")->link_to(n_scr->find_output("images"));
			assert(ok);
			bp->find_input("*.make_cmd.cbs")->set_data_p(cbs);
			auto s_img_idx = bp->find_input("*.make_cmd.image_idx");
			if (s_img_idx)
			{
				ok = s_img_idx->link_to(n_scr->find_output("image_idx"));
				assert(ok);
			}
		}

		SwapchainResizable* SwapchainResizable::create(Device* d, SysWindow* w)
		{
			return new SwapchainResizablePrivate(d, w);
		}

		void SwapchainResizable::destroy(SwapchainResizable* s)
		{
			delete (SwapchainResizablePrivate*)s;
		}

		struct R(R_SwapchainResizable, flame, graphics)
		{
			BP::Node* n;

			BASE0;
			RV(SwapchainResizable*, in, i);

			BASE1;
			RV(Swapchain*, sc, o);
			RV(Array<Image*>, images, o);
			RV(uint, image_idx, o);

			FLAME_GRAPHICS_EXPORTS void RF(active_update)(uint frame)
			{
				if (in->changed)
				{
					sc = in->sc();
					if (sc)
					{
						images.resize(sc->image_count());
						for (auto i = 0; i < images.s; i++)
							images[i] = sc->image(i);
					}
					else
						images.resize(0);
					sc_s()->set_frame(frame);
					images_s()->set_frame(frame);
				}
				image_idx = sc ? sc->image_index() : 0;
				image_idx_s()->set_frame(frame);
				in->changed = false;
			}

			FLAME_GRAPHICS_EXPORTS RF(~R_SwapchainResizable)()
			{
			}
		};
	}
}

