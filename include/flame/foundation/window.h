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

#include <flame/foundation/foundation.h>

namespace flame
{
	struct Application;

	enum WindowStyle
	{
		WindowFrame = 1 << 0,
		WindowResizable = 1 << 1,
		WindowFullscreen = 1 << 2
	};

	enum CursorType
	{
		CursorNone = -1,
		CursorAppStarting, // arrow and small hourglass
		CursorArrow,
		CursorCross, // unknown
		CursorHand,
		CursorHelp,
		CursorIBeam,
		CursorNo,
		CursorSizeAll,
		CursorSizeNESW,
		CursorSizeNS,
		CursorSizeNWSE,
		CursorSizeWE,
		CursorUpArrwo,
		CursorWait,

		CursorCount
	};

	struct Window
	{
		Ivec2 pos;
		Ivec2 size;
		int style;
		String title;

		bool minimized;

		FLAME_FOUNDATION_EXPORTS void *get_native();

#ifdef FLAME_WINDOWS
		FLAME_FOUNDATION_EXPORTS void set_cursor(CursorType type);

		FLAME_FOUNDATION_EXPORTS void set_size(const Ivec2 &_pos, const Ivec2 &_size, int _style);
		FLAME_FOUNDATION_EXPORTS void set_maximized(bool v);
#endif

		FLAME_PACKAGE_BEGIN(KeyListenerParm)
			/*
				- when key down/up, action is KeyStateDown or KeyStateUp, value is Key
				- when char, action is KeyStateNull, value is ch
			*/
			FLAME_PACKAGE_ITEM(KeyState, action, i1)
			FLAME_PACKAGE_ITEM(int, value, i1)
		FLAME_PACKAGE_END

		FLAME_PACKAGE_BEGIN(MouseListenerParm)
			/*
				- when down/up, action is KeyStateDown or KeyStateUp, key is MouseKey
				- when move, action is KeyStateNull, key is Mouse_Null
				- when scroll, action is KeyStateNull, key is Mouse_Middle, pos.x is scroll value
			*/
			FLAME_PACKAGE_ITEM(KeyState, action, i1)
			FLAME_PACKAGE_ITEM(MouseKey, key, i1)
			FLAME_PACKAGE_ITEM(Ivec2, pos, i2)
		FLAME_PACKAGE_END

		FLAME_PACKAGE_BEGIN(ResizeListenerParm)
			FLAME_PACKAGE_ITEM(Ivec2, size, i2)
		FLAME_PACKAGE_END

		FLAME_FOUNDATION_EXPORTS int add_listener(Listener l, PF pf, void *thiz, const std::vector<CommonData> &capt);
		FLAME_FOUNDATION_EXPORTS void remove_listener(Listener l, int idx);

#ifdef FLAME_WINDOWS
		FLAME_FOUNDATION_EXPORTS bool is_modifier_pressing(Key k /* accept: Key_Shift, Key_Ctrl and Key_Alt */, int left_or_right /* 0 or 1 */);

		FLAME_FOUNDATION_EXPORTS static Window *create(Application *app, const char *_title, const Ivec2 &_size, int _style);
#endif

#ifdef FLAME_ANDROID
		FLAME_FOUNDATION_EXPORTS static Window *create(Application *app, void *android_state, void(*callback)());
#endif
		FLAME_FOUNDATION_EXPORTS static void destroy(Window *s);
	};

	struct Application
	{
		long long total_frame;
		long long fps;
		float elapsed_time; // second

		FLAME_FOUNDATION_EXPORTS int run(Function<> &idle_func);

		FLAME_FOUNDATION_EXPORTS void clear_delay_events();
		FLAME_FOUNDATION_EXPORTS void add_delay_event(Function<> &event);

		FLAME_FOUNDATION_EXPORTS static Application *create();
		FLAME_FOUNDATION_EXPORTS static void destroy(Application *m);
	};
}
