// MIT License
// 
// Copyright (c) 2018 wjs
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <flame/type.h>

#include <flame/graphics/graphics.h>

namespace flame
{
	namespace graphics
	{
		struct Device;
		struct Commandbuffer;
		struct Swapchain;
		struct Semaphore;

		struct Queue
		{
			FLAME_GRAPHICS_EXPORTS void wait_idle();
			FLAME_GRAPHICS_EXPORTS void submit(Commandbuffer *c, Semaphore *wait_semaphore, Semaphore *signal_semaphore);
			FLAME_GRAPHICS_EXPORTS void present(Swapchain *s, Semaphore *wait_semaphore);

			FLAME_GRAPHICS_EXPORTS static Queue *create(Device *d, int queue_family_idx);
			FLAME_GRAPHICS_EXPORTS static void destroy(Queue *q);
		};
	}
}
