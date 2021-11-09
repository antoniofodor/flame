#pragma once

#include "foundation_private.h"
#include "system.h"

#define NOMINMAX
#include <Windows.h>

namespace flame
{
	inline KeyboardKey vk_code_to_key(int vkCode)
	{
		switch (vkCode)
		{
		case VK_BACK:
			return Keyboard_Backspace;
		case VK_TAB:
			return Keyboard_Tab;
		case VK_RETURN:
			return Keyboard_Enter;
		case VK_SHIFT:
			return Keyboard_Shift;
		case VK_CONTROL:
			return Keyboard_Ctrl;
		case VK_MENU:
			return Keyboard_Alt;
		case VK_PAUSE:
			return Keyboard_Pause;
		case VK_CAPITAL:
			return Keyboard_CapsLock;
		case VK_ESCAPE:
			return Keyboard_Esc;
		case VK_SPACE:
			return Keyboard_Space;
		case VK_PRIOR:
			return Keyboard_PgUp;
		case VK_NEXT:
			return Keyboard_PgDn;
		case VK_END:
			return Keyboard_End;
		case VK_HOME:
			return Keyboard_Home;
		case VK_LEFT:
			return Keyboard_Left;
		case VK_UP:
			return Keyboard_Up;
		case VK_RIGHT:
			return Keyboard_Right;
		case VK_DOWN:
			return Keyboard_Down;
		case VK_SNAPSHOT:
			return Keyboard_PrtSc;
		case VK_INSERT:
			return Keyboard_Ins;
		case VK_DELETE:
			return Keyboard_Del;
		case '0':
			return Keyboard_0;
		case '1':
			return Keyboard_1;
		case '2':
			return Keyboard_2;
		case '3':
			return Keyboard_3;
		case '4':
			return Keyboard_4;
		case '5':
			return Keyboard_5;
		case '6':
			return Keyboard_6;
		case '7':
			return Keyboard_7;
		case '8':
			return Keyboard_8;
		case '9':
			return Keyboard_9;
		case 'A':
			return Keyboard_A;
		case 'B':
			return Keyboard_B;
		case 'C':
			return Keyboard_C;
		case 'D':
			return Keyboard_D;
		case 'E':
			return Keyboard_E;
		case 'F':
			return Keyboard_F;
		case 'G':
			return Keyboard_G;
		case 'H':
			return Keyboard_H;
		case 'I':
			return Keyboard_I;
		case 'J':
			return Keyboard_J;
		case 'K':
			return Keyboard_K;
		case 'L':
			return Keyboard_L;
		case 'M':
			return Keyboard_M;
		case 'N':
			return Keyboard_N;
		case 'O':
			return Keyboard_O;
		case 'P':
			return Keyboard_P;
		case 'Q':
			return Keyboard_Q;
		case 'R':
			return Keyboard_R;
		case 'S':
			return Keyboard_S;
		case 'T':
			return Keyboard_T;
		case 'U':
			return Keyboard_U;
		case 'V':
			return Keyboard_V;
		case 'W':
			return Keyboard_W;
		case 'X':
			return Keyboard_X;
		case 'Y':
			return Keyboard_Y;
		case 'Z':
			return Keyboard_Z;
		case VK_NUMPAD0:
			return Keyboard_Numpad0;
		case VK_NUMPAD1:
			return Keyboard_Numpad1;
		case VK_NUMPAD2:
			return Keyboard_Numpad2;
		case VK_NUMPAD3:
			return Keyboard_Numpad3;
		case VK_NUMPAD4:
			return Keyboard_Numpad4;
		case VK_NUMPAD5:
			return Keyboard_Numpad5;
		case VK_NUMPAD6:
			return Keyboard_Numpad6;
		case VK_NUMPAD7:
			return Keyboard_Numpad7;
		case VK_NUMPAD8:
			return Keyboard_Numpad8;
		case VK_NUMPAD9:
			return Keyboard_Numpad9;
		case VK_ADD:
			return Keyboard_Add;
		case VK_SUBTRACT:
			return Keyboard_Subtract;
		case VK_MULTIPLY:
			return Keyboard_Multiply;
		case VK_DIVIDE:
			return Keyboard_Divide;
		case VK_SEPARATOR:
			return Keyboard_Separator;
		case VK_DECIMAL:
			return Keyboard_Decimal;
		case VK_F1:
			return Keyboard_F1;
		case VK_F2:
			return Keyboard_F2;
		case VK_F3:
			return Keyboard_F3;
		case VK_F4:
			return Keyboard_F4;
		case VK_F5:
			return Keyboard_F5;
		case VK_F6:
			return Keyboard_F6;
		case VK_F7:
			return Keyboard_F7;
		case VK_F8:
			return Keyboard_F8;
		case VK_F9:
			return Keyboard_F9;
		case VK_F10:
			return Keyboard_F10;
		case VK_F11:
			return Keyboard_F11;
		case VK_F12:
			return Keyboard_F12;
		case VK_NUMLOCK:
			return Keyboard_NumLock;
		case VK_SCROLL:
			return Keyboard_ScrollLock;
		}
		return KeyboardKey_Count;
	}

	inline int key_to_vk_code(KeyboardKey key)
	{
		switch (key)
		{
		case Keyboard_Backspace:
			return VK_BACK;
		case Keyboard_Tab:
			return VK_TAB;
		case Keyboard_Enter:
			return VK_RETURN;
		case Keyboard_Shift:
			return VK_SHIFT;
		case Keyboard_Ctrl:
			return VK_CONTROL;
		case Keyboard_Alt:
			return VK_MENU;
		case Keyboard_Pause:
			return VK_PAUSE;
		case Keyboard_CapsLock:
			return VK_CAPITAL;
		case Keyboard_Esc:
			return VK_ESCAPE;
		case Keyboard_Space:
			return VK_SPACE;
		case Keyboard_PgUp:
			return VK_PRIOR;
		case Keyboard_PgDn:
			return VK_NEXT;
		case Keyboard_End:
			return VK_END;
		case Keyboard_Home:
			return VK_HOME;
		case Keyboard_Left:
			return VK_LEFT;
		case Keyboard_Up:
			return VK_UP;
		case Keyboard_Right:
			return VK_RIGHT;
		case Keyboard_Down:
			return VK_DOWN;
		case Keyboard_PrtSc:
			return VK_SNAPSHOT;
		case Keyboard_Ins:
			return VK_INSERT;
		case Keyboard_Del:
			return VK_DELETE;
		case Keyboard_0:
			return '0';
		case Keyboard_1:
			return '1';
		case Keyboard_2:
			return '2';
		case Keyboard_3:
			return '3';
		case Keyboard_4:
			return '4';
		case Keyboard_5:
			return '5';
		case Keyboard_6:
			return '6';
		case Keyboard_7:
			return '7';
		case Keyboard_8:
			return '8';
		case Keyboard_9:
			return '9';
		case Keyboard_A:
			return 'A';
		case Keyboard_B:
			return 'B';
		case Keyboard_C:
			return 'C';
		case Keyboard_D:
			return 'D';
		case Keyboard_E:
			return 'E';
		case Keyboard_F:
			return 'F';
		case Keyboard_G:
			return 'G';
		case Keyboard_H:
			return 'H';
		case Keyboard_I:
			return 'I';
		case Keyboard_J:
			return 'J';
		case Keyboard_K:
			return 'K';
		case Keyboard_L:
			return 'L';
		case Keyboard_M:
			return 'M';
		case Keyboard_N:
			return 'N';
		case Keyboard_O:
			return 'O';
		case Keyboard_P:
			return 'P';
		case Keyboard_Q:
			return 'Q';
		case Keyboard_R:
			return 'R';
		case Keyboard_S:
			return 'S';
		case Keyboard_T:
			return 'T';
		case Keyboard_U:
			return 'U';
		case Keyboard_V:
			return 'V';
		case Keyboard_W:
			return 'W';
		case Keyboard_X:
			return 'X';
		case Keyboard_Y:
			return 'Y';
		case Keyboard_Z:
			return 'Z';
		case Keyboard_Numpad0:
			return VK_NUMPAD0;
		case Keyboard_Numpad1:
			return VK_NUMPAD1;
		case Keyboard_Numpad2:
			return VK_NUMPAD2;
		case Keyboard_Numpad3:
			return VK_NUMPAD3;
		case Keyboard_Numpad4:
			return VK_NUMPAD4;
		case Keyboard_Numpad5:
			return VK_NUMPAD5;
		case Keyboard_Numpad6:
			return VK_NUMPAD6;
		case Keyboard_Numpad7:
			return VK_NUMPAD7;
		case Keyboard_Numpad8:
			return VK_NUMPAD8;
		case Keyboard_Numpad9:
			return VK_NUMPAD9;
		case Keyboard_Add:
			return VK_ADD;
		case Keyboard_Subtract:
			return VK_SUBTRACT;
		case Keyboard_Multiply:
			return VK_MULTIPLY;
		case Keyboard_Divide:
			return VK_DIVIDE;
		case Keyboard_Separator:
			return VK_SEPARATOR;
		case Keyboard_Decimal:
			return VK_DECIMAL;
		case Keyboard_F1:
			return VK_F1;
		case Keyboard_F2:
			return VK_F2;
		case Keyboard_F3:
			return VK_F3;
		case Keyboard_F4:
			return VK_F4;
		case Keyboard_F5:
			return VK_F5;
		case Keyboard_F6:
			return VK_F6;
		case Keyboard_F7:
			return VK_F7;
		case Keyboard_F8:
			return VK_F8;
		case Keyboard_F9:
			return VK_F9;
		case Keyboard_F10:
			return VK_F10;
		case Keyboard_F11:
			return VK_F11;
		case Keyboard_F12:
			return VK_F12;
		case Keyboard_NumLock:
			return VK_NUMLOCK;
		case Keyboard_ScrollLock:
			return VK_SCROLL;
		}
		return KeyboardKey_Count;
	}
}
