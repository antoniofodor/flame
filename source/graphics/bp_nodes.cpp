//MIT License
//
//Copyright (c) 2018 wjs
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include <flame/graphics/device.h>
#include <flame/graphics/renderpass.h>
#include <flame/graphics/swapchain.h>
#include <flame/graphics/commandbuffer.h>
#include <flame/graphics/bp_nodes.h>

namespace flame
{
#define CODE \
		if (in$i)\
			out$o = in$i;

	void BP_GraphicsDevice::update()
	{
		CODE
	}

	const char* BP_GraphicsDevice::code()
	{
		return FLAME_STR(CODE);
	}

#undef CODE

	BP_GraphicsDevice bp_graphics_device_unused;

#define CODE \
		if (in$i)\
		{\
			out$o = in$i;\
			auto sc = (graphics::Swapchain*)out$o;\
			window$o = sc->window();\
			image1$o = sc->get_image(0);\
			image2$o = sc->get_image(1);\
			renderpass_clear$o = sc->get_renderpass_clear();\
			renderpass_dont_clear$o = sc->get_renderpass_dont_clear();\
			framebuffer1$o = sc->get_framebuffer(0);\
			framebuffer2$o = sc->get_framebuffer(1);\
		}

	void BP_GraphicsSwapchain::update()
	{
		CODE
	}

	const char* BP_GraphicsSwapchain::code()
	{
		return FLAME_STR(CODE);
	}

#undef CODE

	BP_GraphicsSwapchain bp_graphics_swapchain_unused;

#define CODE \
		if (in$i)\
			out$o = in$i;\
		else\
		{\
			if (renderpass$i)\
				out$o = graphics::ClearValues::create((graphics::Renderpass*)renderpass$i);\
		}\
		if (out$o)\
		{\
			for (auto i = 0; i < colors$i.size; i++)\
			{\
				auto cv = (graphics::ClearValues*)out$o;\
				cv->set(i, colors$i[i]);\
			}\
		}

	void BP_GraphicsClearvalues::update()
	{
		CODE
	}

	const char* BP_GraphicsClearvalues::code()
	{
		return FLAME_STR(CODE);
	}

#undef CODE

	BP_GraphicsClearvalues bp_graphics_clearvalues_unused;

#define CODE \
		if (in$i)\
			out$o = in$i;\
		else\
		{\
			if (device$i)\
				out$o = graphics::Commandbuffer::create(((graphics::Device*)device$i)->gcp);\
		}

	void BP_GraphicsCommandbuffer::update()
	{
		CODE
	}

	const char* BP_GraphicsCommandbuffer::code()
	{
		return FLAME_STR(CODE);
	}

	BP_GraphicsCommandbuffer bp_graphics_commandbuffer_unused;

#define CODE \
		if (cmd1$i)\
			((graphics::Commandbuffer*)cmd1$i)->begin();\
		if (cmd2$i)\
			((graphics::Commandbuffer*)cmd2$i)->begin();

	void BP_GraphicsCmdBegin::update()
	{
		CODE
	}

	const char* BP_GraphicsCmdBegin::code()
	{
		return FLAME_STR(CODE);
	}

	BP_GraphicsCmdBegin bp_graphics_cmd_begin_unused;

#define CODE \
		if (cmd1$i)\
			((graphics::Commandbuffer*)cmd1$i)->end();\
		if (cmd2$i)\
			((graphics::Commandbuffer*)cmd2$i)->end();

	void BP_GraphicsCmdEnd::update()
	{
		CODE
	}

	const char* BP_GraphicsCmdEnd::code()
	{
		return FLAME_STR(CODE);
	}

	BP_GraphicsCmdEnd bp_graphics_cmd_end_unused;

#define CODE \
		if (cmd1$i)\
			((graphics::Commandbuffer*)cmd1$i)->begin_renderpass((graphics::Renderpass*)renderpass$i, (graphics::Framebuffer*)framebuffer1$i, (graphics::ClearValues*)clearvalues$i);\
		if (cmd2$i)\
			((graphics::Commandbuffer*)cmd2$i)->begin_renderpass((graphics::Renderpass*)renderpass$i, (graphics::Framebuffer*)framebuffer2$i, (graphics::ClearValues*)clearvalues$i);\

	void BP_GraphicsCmdBeginRenderpass::update()
	{
		CODE
	}

	const char* BP_GraphicsCmdBeginRenderpass::code()
	{
		return FLAME_STR(CODE);
	}

	BP_GraphicsCmdBeginRenderpass bp_graphics_cmd_begin_renderpass_unused;

#define CODE \
		if (cmd1$i)\
			((graphics::Commandbuffer*)cmd1$i)->end_renderpass();\
		if (cmd2$i)\
			((graphics::Commandbuffer*)cmd2$i)->end_renderpass();

	void BP_GraphicsCmdEndRenderpass::update()
	{
		CODE
	}

	const char* BP_GraphicsCmdEndRenderpass::code()
	{
		return FLAME_STR(CODE);
	}

	BP_GraphicsCmdEndRenderpass bp_graphics_cmd_end_renderpass_unused;
}
