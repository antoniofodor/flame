#pragma once

#include <stdarg.h>
#include <assert.h>
#include <string>
#include <array>
#include <vector>
#include <span>
#include <stack>
#include <list>
#include <map>
#include <unordered_map>
#include <memory>

namespace flame
{
	typedef char*				charptr;
	typedef wchar_t*			wcharptr;
	typedef unsigned char		uchar;
	typedef unsigned short		ushort;
	typedef unsigned int		uint;
	typedef long long			int64;
	typedef unsigned long long  uint64;
	typedef void*				voidptr;

	const auto INVALID_POINTER = (void*)0x7fffffffffffffff;

	template<class T, size_t N>
	constexpr size_t size(T(&)[N]) { return N; }

	template <class T>
	void* var_end(T* p)
	{
		return (char*)p + sizeof(T);
	}

	template <auto V>
	struct S
	{
		constexpr static decltype(V) v = V;
	};

	template <class T>
	std::span<T> SP(const T& v)
	{
		return std::span(&(T&)v, 1);
	}

	uint64 constexpr ch(char const* str)
	{
		auto ret = std::_FNV_offset_basis;
		while (*str)
		{
			ret ^= *str;
			ret *= std::_FNV_prime;
			str++;
		}
		return ret;
	}

	template <class F>
	void* f2a(F f) // function to address
	{
		union
		{
			F f;
			void* p;
		}cvt;
		cvt.f = f;
		return cvt.p;
	}

	template <class F>
	F a2f(void* p) // address to function
	{
		union
		{
			void* p;
			F f;
		}cvt;
		cvt.p = p;
		return cvt.f;
	}

	struct CountDown
	{
		bool is_frame;

		union
		{
			uint frames;
			float time;
		}v;

		CountDown() :
			is_frame(true)
		{
			v.frames = 0;
		}

		CountDown(uint frames) :
			is_frame(true)
		{
			v.frames = frames;
		}

		CountDown(float time) :
			is_frame(false)
		{
			v.time = time;
		}
	};
}
