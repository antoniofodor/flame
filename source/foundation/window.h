#pragma once

#include "foundation.h"

namespace flame
{
	enum WindowStyleFlags
	{
		WindowFrame = 1 << 0,
		WindowResizable = 1 << 1,
		WindowFullscreen = 1 << 2,
		WindowMaximized = 1 << 3,
		WindowTopmost = 1 << 4
	};

	inline WindowStyleFlags operator| (WindowStyleFlags a, WindowStyleFlags b) { return (WindowStyleFlags)((int)a | (int)b); }

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

		Cursor_Count
	};

	struct NativeWindow
	{
		ivec2 pos;
		uvec2 size;
		std::string title;
		uint style;

		CursorType cursor = CursorArrow;

		Listeners<void(KeyboardKey)> key_down_listeners;
		Listeners<void(KeyboardKey)> key_up_listeners;
		Listeners<void(wchar_t)> char_listeners;
		Listeners<void(const ivec2&)> mouse_left_down_listeners;
		Listeners<void(const ivec2&)> mouse_left_up_listeners;
		Listeners<void(const ivec2&)> mouse_right_down_listeners;
		Listeners<void(const ivec2&)> mouse_right_up_listeners;
		Listeners<void(const ivec2&)> mouse_middle_down_listeners;
		Listeners<void(const ivec2&)> mouse_middle_up_listeners;
		Listeners<void(const ivec2&)> mouse_move_listeners;
		Listeners<void(int)> mouse_scroll_listeners;
		Listeners<void(const uvec2&)> resize_listeners;
		Listeners<void()> destroy_listeners;
		
		bool has_input = false;

		void* userdata = nullptr;

		virtual void* get_hwnd() = 0;

		virtual void close() = 0;

		virtual void set_pos(const ivec2& pos) = 0;
		virtual void set_size(const uvec2& size) = 0;
		virtual ivec2 global_to_local(const ivec2& p) = 0;
		virtual void set_title(std::string_view title) = 0;
		virtual void set_cursor(CursorType type) = 0;

		struct Create
		{
			virtual NativeWindowPtr operator()(std::string_view title, const uvec2& size, WindowStyleFlags style, NativeWindowPtr parent = nullptr) = 0;
		};
		FLAME_FOUNDATION_EXPORTS static Create& create;
	};

	FLAME_FOUNDATION_EXPORTS const std::vector<NativeWindowPtr>& get_windows();
}
