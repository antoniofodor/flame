#pragma once

#include "type.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/color_space.hpp>

using namespace glm;

namespace flame
{
	typedef vec<2, uchar> cvec2;
	typedef vec<3, uchar> cvec3;
	typedef vec<4, uchar> cvec4;

	template<uint N, typename T>
	inline T sum(const vec<N, T>& v)
	{
		T ret = 0;
		for (auto i = 0; i < N; i++)
			ret += v[i];
		return ret;
	}

	template<uint N, typename T>
	inline T product(const vec<N, T>& v)
	{
		T ret = v[0];
		for (auto i = 1; i < N; i++)
			ret *= v[i];
		return ret;
	}

	inline float square(float v)
	{
		return v * v;
	}

	template<typename T, typename ...Args>
	T minN(T a, T b, Args... args)
	{
		return minN(min(a, b), args...);
	}

	template<typename T, typename ...Args>
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

	inline float angle_diff(float ang0, float ang1)
	{
		ang0 = mod(ang0, 360.f); if (ang0 < 0.f) ang0 += 360.f;
		ang1 = mod(ang1, 360.f); if (ang1 < 0.f) ang1 += 360.f;
		auto diff1 = ang0 - ang1; if (diff1 < 0.f) diff1 += 360.f;
		auto diff2 = ang1 - ang0; if (diff2 < 0.f) diff2 += 360.f;
		if (diff1 < diff2)
			return -diff1;
		return diff2;
	}

	inline float sign_min(float a, float b)
	{
		return sign(a) * min(abs(a), abs(b));
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

	union LightCommonValue
	{
		cvec4 c;
		int i;
		uint u;
		float f;
	};

	union CommonValue
	{
		cvec4 c;
		ivec4 i;
		uvec4 u;
		vec4 f;
		void* p;
	};

	inline float triangle_area(const vec2& a, const vec2& b, const vec2& c)
	{
		return ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) / 2.f;
	}

	inline vec2 convex_centroid(std::span<vec2> points)
	{
		vec2 ret(0.f); float area = 0.f;
		if (points.size() < 3)
			return ret;

		for (auto i = 1; i < points.size() - 1; i++)
		{
			auto m = (points[0] + points[i] + points[i + 1]) / 3.f;
			auto a = triangle_area(points[0], points[1], points[2]);
			ret = (ret * area + m * a) / (area + a);
			area += a;
		}

		return ret;
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

	inline float segment_intersect(const vec2& a, const vec2& b, const vec2& c, const vec2& d)
	{
		auto ab = b - a;
		auto dc = c - d;
		return cross2(ab, c - a) * cross2(ab, d - a) <= 0.f &&
			cross2(dc, a - d) * cross2(dc, b - d) <= 0.f;
	}

	inline vec3 smooth_damp(const vec3& current, const vec3& _target, vec3& current_velocity, float smooth_time, float max_speed, float delta_time)
	{
		auto target = _target;

		float output_x = 0.f;
		float output_y = 0.f;
		float output_z = 0.f;

		// Based on Game Programming Gems 4 Chapter 1.10
		smooth_time = max(0.0001f, smooth_time);
		float omega = 2.f / smooth_time;

		float x = omega * delta_time;
		float exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);

		float change_x = current.x - target.x;
		float change_y = current.y - target.y;
		float change_z = current.z - target.z;
		auto original_to = target;

		// Clamp maximum speed
		float max_change = max_speed * smooth_time;

		float max_change_sq = max_change * max_change;
		float sqrmag = change_x * change_x + change_y * change_y + change_z * change_z;
		if (sqrmag > max_change_sq)
		{
			auto mag = sqrt(sqrmag);
			change_x = change_x / mag * max_change;
			change_y = change_y / mag * max_change;
			change_z = change_z / mag * max_change;
		}

		target.x = current.x - change_x;
		target.y = current.y - change_y;
		target.z = current.z - change_z;

		float temp_x = (current_velocity.x + omega * change_x) * delta_time;
		float temp_y = (current_velocity.y + omega * change_y) * delta_time;
		float temp_z = (current_velocity.z + omega * change_z) * delta_time;

		current_velocity.x = (current_velocity.x - omega * temp_x) * exp;
		current_velocity.y = (current_velocity.y - omega * temp_y) * exp;
		current_velocity.z = (current_velocity.z - omega * temp_z) * exp;

		output_x = target.x + (change_x + temp_x) * exp;
		output_y = target.y + (change_y + temp_y) * exp;
		output_z = target.z + (change_z + temp_z) * exp;

		// Prevent overshooting
		float origMinusCurrent_x = original_to.x - current.x;
		float origMinusCurrent_y = original_to.y - current.y;
		float origMinusCurrent_z = original_to.z - current.z;
		float outMinusOrig_x = output_x - original_to.x;
		float outMinusOrig_y = output_y - original_to.y;
		float outMinusOrig_z = output_z - original_to.z;

		if (origMinusCurrent_x * outMinusOrig_x + origMinusCurrent_y * outMinusOrig_y + origMinusCurrent_z * outMinusOrig_z > 0)
		{
			output_x = original_to.x;
			output_y = original_to.y;
			output_z = original_to.z;

			current_velocity.x = (output_x - original_to.x) / delta_time;
			current_velocity.y = (output_y - original_to.y) / delta_time;
			current_velocity.z = (output_z - original_to.z) / delta_time;
		}

		return vec3(output_x, output_y, output_z);
	}

	struct Rect
	{
		vec2 a;
		vec2 b;

		Rect()
		{
			reset();
		}

		Rect(float LT_x, float LT_y, float RB_x, float RB_y)
		{
			a.x = LT_x;
			a.y = LT_y;
			b.x = RB_x;
			b.y = RB_y;
		}

		Rect(const vec2& a, const vec2& b) :
			Rect(a.x, a.y, b.x, b.y)
		{
		}

		Rect(const vec4& v) :
			Rect(v.x, v.y, v.z, v.w)
		{
		}

		void reset()
		{
			a = vec2(10000.f);
			b = vec2(-10000.f);
		}

		operator vec4() const
		{
			return vec4(a.x, a.y, b.x, b.y);
		}

		bool operator==(const Rect& rhs)
		{
			return a.x == rhs.a.x && a.y == rhs.a.y &&
				b.x == rhs.b.x && b.y == rhs.b.y;
		}

		void expand(float length)
		{
			a.x -= length;
			a.y += length;
			b.x -= length;
			b.y += length;
		}

		void expand(const vec2& p)
		{
			a = min(a, p);
			b = max(b, p);
		}

		bool contains(const vec2& p)
		{
			return p.x > a.x && p.x < b.x &&
				p.y > a.y && p.y < b.y;
		}

		bool overlapping(const Rect& rhs)
		{
			return !(rhs.a.x > b.x || rhs.a.y > b.y);
		}
	};

	struct AABB
	{
		vec3 a;
		vec3 b;

		AABB()
		{
			reset();
		}

		AABB(const vec3& a, const vec3& b) :
			a(a),
			b(b)
		{
		}

		AABB(const vec3& center, float size)
		{
			auto hf_size = size * 0.5f;
			a = center - hf_size;
			b = center + hf_size;
		}

		AABB(uint count, const vec3* points, const mat3& mat = mat3(1.f))
		{
			reset();
			for (auto i = 0; i < count; i++)
				expand(mat * points[i]);
		}

		AABB(const std::vector<vec3>& points, const mat3& mat = mat3(1.f)) :
			AABB(points.size(), points.data(), mat)
		{
		}

		void reset()
		{
			a = vec3(+10000.f);
			b = vec3(-10000.f);
		}

		bool invalid()
		{
			return any(greaterThan(a, b));
		}

		vec3 center() const
		{
			return (a + b) * 0.5f;
		}

		float radius() const
		{
			return distance(a, b) * 0.5f;
		}

		void get_points(vec3* dst, const mat4& m = mat4(1.f)) const
		{
			dst[0] = m * vec4(a.x, a.y, a.z, 1.f);
			dst[1] = m * vec4(b.x, a.y, a.z, 1.f);
			dst[2] = m * vec4(b.x, a.y, b.z, 1.f);
			dst[3] = m * vec4(a.x, a.y, b.z, 1.f);
			dst[4] = m * vec4(a.x, b.y, a.z, 1.f);
			dst[5] = m * vec4(b.x, b.y, a.z, 1.f);
			dst[6] = m * vec4(b.x, b.y, b.z, 1.f);
			dst[7] = m * vec4(a.x, b.y, b.z, 1.f);
		}

		std::vector<vec3> get_points(const mat4& m = mat4(1.f)) const
		{
			std::vector<vec3> ret;
			ret.resize(8);
			get_points(ret.data(), m);
			return ret;
		}

		void expand(const vec3& p)
		{
			a = min(a, p);
			b = max(b, p);
		}

		void expand(const AABB& oth)
		{
			a = min(a, oth.a);
			b = max(b, oth.b);
		}

		bool contains(const vec3& p)
		{
			return all(greaterThan(p, a)) && all(greaterThan(b, p));
		}

		bool contains(const AABB& oth)
		{
			return contains(oth.a) && contains(oth.b);
		}

		bool intersects(const AABB& oth)
		{
			return !(any(greaterThan(oth.a, b)) || any(lessThan(oth.b, a)));
		}

		bool intersects(const vec3& center, float radius)
		{
			auto d = 0.f;
			if		(center.x < a.x) d += square(center.x - a.x);
			else if (center.x > b.x) d += square(center.x - b.x);
			if		(center.y < a.y) d += square(center.y - a.y);
			else if (center.y > b.y) d += square(center.y - b.y);
			if		(center.z < a.z) d += square(center.z - a.z);
			else if (center.z > b.z) d += square(center.z - b.z);
			return square(radius) > d;
		}

		bool intersects(const vec2& center, float radius)
		{
			auto d = 0.f;
			if		(center.x < a.x) d += square(center.x - a.x);
			else if (center.x > b.x) d += square(center.x - b.x);
			if		(center.y < a.z) d += square(center.y - a.z);
			else if (center.y > b.z) d += square(center.y - b.z);
			return square(radius) > d;
		}
	};

	struct Plane
	{
		vec3 n;
		float d;

		Plane() = default;

		Plane(const vec3 &n, float d) :
			n(n),
			d(d)
		{
		}

		Plane(const vec3& n, const vec3& p) :
			n(n),
			d(-dot(n, p))
		{
		}

		Plane(const vec3& a, const vec3& b, const vec3& c)
		{
			n = normalize(cross(b - a, c - a));
			d = -dot(n, a);
		}

		float distance(const vec3& p) const
		{
			return dot(n, p) + d;
		}
	};

	struct Frustum
	{
		Plane planes[6];

		Frustum() = default;

		void set(const vec3* points)
		{
			planes[0] = Plane(points[0], points[1], points[2]); // near
			planes[1] = Plane(points[4], points[7], points[5]); // far
			planes[2] = Plane(points[3], points[7], points[4]); // left
			planes[3] = Plane(points[1], points[5], points[6]); // right
			planes[4] = Plane(points[0], points[4], points[5]); // top
			planes[5] = Plane(points[2], points[6], points[7]); // bottom
		}

		static std::vector<vec3> get_points(const mat4& inv, float n = 0.f, float f = 1.f)
		{
			std::vector<vec3> ret;
			ret.resize(8);
			auto trans_point = [&](const vec3& p) {
				auto ret = inv * vec4(p, 1.f);
				ret /= ret.w;
				return ret;
			};
			ret[0] = trans_point(vec3(-1.f, -1.f, n));
			ret[1] = trans_point(vec3(+1.f, -1.f, n));
			ret[2] = trans_point(vec3(+1.f, +1.f, n));
			ret[3] = trans_point(vec3(-1.f, +1.f, n));
			ret[4] = trans_point(vec3(-1.f, -1.f, f));
			ret[5] = trans_point(vec3(+1.f, -1.f, f));
			ret[6] = trans_point(vec3(+1.f, +1.f, f));
			ret[7] = trans_point(vec3(-1.f, +1.f, f));
			return ret;
		}

		static std::vector<vec3> points_to_lines(const vec3* points)
		{
			std::vector<vec3> ret;
			ret.resize(24);
			auto p = ret.data();
			*p++ = points[0]; *p++ = points[1];
			*p++ = points[1]; *p++ = points[2];
			*p++ = points[2]; *p++ = points[3];
			*p++ = points[3]; *p++ = points[0];
			*p++ = points[0]; *p++ = points[4];
			*p++ = points[1]; *p++ = points[5];
			*p++ = points[2]; *p++ = points[6];
			*p++ = points[3]; *p++ = points[7];
			*p++ = points[4]; *p++ = points[5];
			*p++ = points[5]; *p++ = points[6];
			*p++ = points[6]; *p++ = points[7];
			*p++ = points[7]; *p++ = points[4];
			return ret;
		}

		Frustum(const vec3* points)
		{
			set(points);
		}

		Frustum(const mat4& inv)
		{
			set(get_points(inv).data());
		}
	};

	inline bool AABB_frustum_check(const Frustum& frustum, const AABB& bounds)
	{
		vec3 ps[8];
		bounds.get_points(ps);
		for (auto i = 0; i < 6; i++)
		{
			auto outside = true;
			for (auto j = 0; j < 8; j++)
			{
				if (frustum.planes[i].distance(ps[j]) > 0.f)
				{
					outside = false;
					break;
				}
			}
			if (outside)
				return false;
		}
		return true;
	}

	struct PerspectiveProjector
	{
		vec2 screen_sz;
		float aspect;
		float fovy;
		float zNear;
		float zFar;
		float tanfovy;

		mat4 matp;

		void set(const vec2& sz, float fov, float n, float f)
		{
			screen_sz = sz;
			aspect = screen_sz.x / screen_sz.y;
			fovy = radians(fov);
			zNear = n;
			zFar = f;
			tanfovy = tan(fovy);

			matp = perspective(fovy, aspect, zNear, zFar);
		}

		vec2 proj(const vec3& p)
		{
			auto pp = matp * vec4(p, 1.f);
			pp /= pp.w;
			return (pp.xy() * 0.5f + 0.5f) * screen_sz;
		}

	};

	using basic_math_types = type_list<ivec2, ivec3, ivec4, uvec2, uvec3, uvec4, cvec2, cvec3, cvec4, vec2, vec3, vec4, mat2, mat3, mat4, quat, 
		Rect, AABB, Plane, Frustum>;

	template<typename T>
	concept basic_math_type = is_one_of_t<T>(basic_math_types());
}

