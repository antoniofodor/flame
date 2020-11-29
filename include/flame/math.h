#pragma once

#include <flame/type.h>

#include <algorithm>

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
using namespace glm;

namespace flame
{
	const float EPS = 0.000001f;

	typedef vec<2, uchar> cvec2;
	typedef vec<3, uchar> cvec3;
	typedef vec<4, uchar> cvec4;

	template <class T, class ...Args>
	T minN(T a, T b, Args... args)
	{
		return minN(min(a, b), args...);
	}

	template <class T, class ...Args>
	T maxN(T a, T b, Args... rest)
	{
		return maxN(max(a, b), rest...);
	}

	inline float cross2(const vec2& a, const vec2& b)
	{
		return a.x * b.y - a.y * b.x;
	}

	inline uint image_pitch(uint b)
	{
		return (uint)ceil((b / 4.f)) * 4U;
	}

	enum SideFlags
	{
		Outside = 0,
		SideN = 1 << 0,
		SideS = 1 << 1,
		SideW = 1 << 2,
		SideE = 1 << 3,
		SideNW = 1 << 4,
		SideNE = 1 << 5,
		SideSW = 1 << 6,
		SideSE = 1 << 7,
		SideCenter = 1 << 8,
		Inside
	};

	inline SideFlags operator| (SideFlags a, SideFlags b) { return (SideFlags)((int)a | (int)b); }

	union CommonValue
	{
		cvec4 c;
		ivec4 i;
		uvec4 u;
		vec4 f;
		void* p;
	};

	template <uint N>
	CommonValue common(const vec<N, uint, lowp>& v)
	{
		CommonValue cv;
		for (auto i = 0; i < N; i++)
			cv.c[i] = v[i];
		return cv;
	}

	template <uint N>
	CommonValue common(const vec<N, int, highp>& v)
	{
		CommonValue cv;
		for (auto i = 0; i < N; i++)
			cv.i[i] = v[i];
		return cv;
	}

	template <uint N>
	CommonValue common(const vec<N, uint, highp>& v)
	{
		CommonValue cv;
		for (auto i = 0; i < N; i++)
			cv.u[i] = v[i];
		return cv;
	}

	template <uint N>
	CommonValue common(const vec<N, float, highp>& v)
	{
		CommonValue cv;
		for (auto i = 0; i < N; i++)
			cv.f[i] = v[i];
		return cv;
	}

	inline CommonValue common(void* p)
	{
		CommonValue cv;
		cv.p = p;
		return cv;
	}

	inline float segment_intersect(const vec2& a, const vec2& b, const vec2& c, const vec2& d)
	{
		auto ab = b - a;
		auto dc = c - d;
		return cross2(ab, c - a) * cross2(ab, d - a) <= 0.f &&
			cross2(dc, a - d) * cross2(dc, b - d) <= 0.f;
	}

	inline bool convex_contains(const vec2& p, std::span<vec2> points)
	{
		if (points.size() < 3)
			return false;

		if (cross(vec3(p - points[0], 0), vec3(points[1] - p, 0)).z > 0.f)
			return false;
		if (cross(vec3(p - points[points.size() - 1], 0), vec3(points[0] - p, 0)).z > 0.f)
			return false;

		for (auto i = 1; i < points.size() - 1; i++)
		{
			if (cross(vec3(p - points[i], 0), vec3(points[i + 1] - p, 0)).z > 0.f)
				return false;
		}

		return true;
	}

	struct Rect
	{
		vec2 LT;
		vec2 RB;

		Rect()
		{
			reset();
		}

		Rect(float LT_x, float LT_y, float RB_x, float RB_y)
		{
			LT.x = LT_x;
			LT.y = LT_y;
			RB.x = RB_x;
			RB.y = RB_y;
		}

		void reset()
		{
			LT = vec2(-10000.f);
			RB = vec2(10000.f);
		}

		bool operator==(const Rect& rhs)
		{
			return LT.x == rhs.LT.x && LT.y == rhs.LT.y &&
				RB.x == rhs.RB.x && RB.y == rhs.RB.y;
		}

		void expand(float length)
		{
			LT.x -= length;
			LT.y += length;
			RB.x -= length;
			RB.y += length;
		}

		void expand(const vec2& p)
		{
			LT.x = min(LT.x, p.x);
			LT.y = max(LT.y, p.x);
			RB.x = min(RB.x, p.y);
			RB.y = max(RB.y, p.y);
		}

		bool contains(const vec2& p)
		{
			return p.x > LT.x && p.x < LT.y &&
				p.y > RB.x && p.y < RB.y;
		}

		bool overlapping(const Rect& rhs)
		{
			return LT.x <= rhs.RB.x && RB.x >= rhs.LT.x &&
				LT.y <= rhs.RB.y && RB.y >= rhs.LT.y;
		}
	};

	// (x, y z) - (yaw pitch roll)

	//template <class T>
	//Vec<3, T> eulerYPR(const Vec<4, T>& q)
	//{
	//	Vec<3, T> ret;

	//	T sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	//	T cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	//	ret.z = atan2(sinr_cosp, cosr_cosp); 

	//	T sinp = 2 * (q.w * q.y - q.z * q.x);
	//	if (abs(sinp) >= 1)
	//		ret.y = copysign(pi<float>() / 2, sinp); // use 90 degrees if out of range
	//	else
	//		ret.y = asin(sinp);

	//	T siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	//	T cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	//	ret.x = atan2(siny_cosp, cosy_cosp);

	//	return ret;
	//}

	//template <class T>
	//Mat<3, 3, T> rotation(const Vec<3, T>& e /* yaw pitch roll */)
	//{
	//	Mat<3, 3, T> ret(T(1));

	//	Mat<3, 3, T> mat_yaw(ret[1], radians(e.x));
	//	ret[0] = mat_yaw * ret[0];
	//	ret[2] = mat_yaw * ret[2];
	//	Mat<3, 3, T> mat_pitch(ret[0], radians(e.y));
	//	ret[2] = mat_pitch * ret[2];
	//	ret[1] = mat_pitch * ret[1];
	//	Mat<3, 3, T> mat_roll(ret[2], radians(e.z));
	//	ret[1] = mat_roll * ret[1];
	//	ret[0] = mat_roll * ret[0];

	//	return ret;
	//}

	struct Plane
	{
		// Ax + Bx + Cz + D = 0

		//template <class T>
		//T plane_intersect(const Vec<4, T>& plane, const Vec<3, T>& origin, const Vec<3, T>& dir)
		//{
		//	auto normal = Vec<3, T>(plane);
		//	auto numer = dot(normal, origin) - plane.w;
		//	auto denom = dot(normal, dir);

		//	if (abs(denom) < EPS)
		//		return -1.f;

		//	return -(numer / denom);
		//}
	};

	struct AABB
	{
		// d[0] - min, d[1] - max

		//template <class T>
		//void AABB_offset(Mat<2, 3, T>& AABB, const Vec<3, T>& off)
		//{
		//	AABB.d[0] += off;
		//	AABB.d[1] += off;
		//}

		//template <class T>
		//void AABB_merge(Mat<2, 3, T>& AABB, const Vec<3, T>& p)
		//{
		//	auto& v1 = AABB.d[0], v2 = AABB.d[1];
		//	v1.x = min(v1.x, p.x);
		//	v1.y = min(v1.y, p.y);
		//	v1.z = min(v1.z, p.z);
		//	v2.x = max(v2.x, p.x);
		//	v2.y = max(v2.y, p.y);
		//	v2.z = max(v2.z, p.z);
		//}

		//template <class T>
		//void AABB_points(const Mat<2, 3, T>& AABB, Vec<3, T>* dst)
		//{
		//	auto& v1 = AABB.d[0], v2 = AABB.d[1];
		//	dst[0] = v1;
		//	dst[1] = Vec<3, T>(v2.x, v1.y, v1.z);
		//	dst[2] = Vec<3, T>(v2.x, v1.y, v2.z);
		//	dst[3] = Vec<3, T>(v1.x, v1.y, v2.z);
		//	dst[4] = Vec<3, T>(v1.x, v2.y, v1.z);
		//	dst[5] = Vec<3, T>(v2.x, v2.y, v1.z);
		//	dst[6] = v2;
		//	dst[7] = Vec<3, T>(v1.x, v2.y, v2.z);
		//}
	};
}

