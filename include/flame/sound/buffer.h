#pragma once

#include <flame/sound/sound.h>

namespace flame
{
	namespace sound
	{
		struct Buffer
		{
			FLAME_SOUND_EXPORTS static Buffer* create_from_data(int size, void* data);
			FLAME_SOUND_EXPORTS static Buffer *create_from_file(const wchar_t *filename, bool reverse = false);
			FLAME_SOUND_EXPORTS static void destroy(Buffer *b);
		};
	}
}
