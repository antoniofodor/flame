#pragma once

#ifdef FLAME_UNIVERSE_MODULE
#define FLAME_UNIVERSE_EXPORTS __declspec(dllexport)
#else
#define FLAME_UNIVERSE_EXPORTS __declspec(dllimport)
#endif

#include <flame/foundation/foundation.h>

namespace flame
{
	enum FLAME_RE(StateFlags)
	{
		StateNone = 0,
		StateHovering = 1 << 0,
		StateFocusing = 1 << 1,
		StateActive = 1 << 2,
		StateSelected = 1 << 3
	};

	inline StateFlags operator| (StateFlags a, StateFlags b) { return (StateFlags)((int)a | (int)b); }

	enum FocusType
	{
		FocusByLeftButton,
		FocusByRightButton,
		FocusByLeftOrRightButton
	};

	enum AlignFlags
	{
		AlignMin = 1 << 0,
		AlignMax = 1 << 1,
		AlignMiddle = 1 << 2,
		AlignMinMax = 1 << 3,
		AlignAbsolute = 1 << 4, // no padding
		AlignGreedy = 1 << 5
	};

	inline AlignFlags operator| (AlignFlags a, AlignFlags b) { return (AlignFlags)((int)a | (int)b); }

	enum LayoutType
	{
		LayoutFree,
		LayoutVertical,
		LayoutHorizontal,
		LayoutGrid
	};

	enum ScrollbarType
	{
		ScrollbarVertical,
		ScrollbarHorizontal
	};

	enum SplitterType
	{
		SplitterHorizontal,
		SplitterVertical
	};

	enum ExtraDrawFlags
	{
		ExtraDrawFilledCornerSE = 1 << 0,
		ExtraDrawHorizontalLine = 1 << 1,
		ExtraDrawVerticalLine = 1 << 2
	};

	inline ExtraDrawFlags operator| (ExtraDrawFlags a, ExtraDrawFlags b) { return (ExtraDrawFlags)((int)a | (int)b); }
}
