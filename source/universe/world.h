#pragma once

#include "universe.h"

namespace flame
{
	struct World
	{
		virtual void release() = 0;

		virtual System* get_system(uint type_hash) const = 0;
		virtual System* find_system(const char* name) const = 0;
		template <class T> inline T* get_system_t() const { return (T*)get_system(T::type_hash); }

		virtual void add_system(System* s) = 0;
		virtual void remove_system(System* s) = 0;

		virtual Entity* get_root() = 0;

		FLAME_UNIVERSE_EXPORTS static World* create();
	};
}
