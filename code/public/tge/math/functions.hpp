#pragma once

#include "types.hpp"
#include "constants.hpp"
#include <algorithm>

namespace Tge::Math
{
// Angle conversion
inline constexpr float ToRadians(float degrees)
{
	return degrees * DegToRad;
}

inline constexpr float ToDegrees(float radians)
{
	return radians * RadToDeg;
}

// Common math functions
template<typename T>
inline constexpr T Clamp(T value, T min, T max)
{
	return glm::clamp(value, min, max);
}

template<typename T>
inline constexpr T Lerp(T a, T b, float t)
{
	return a + (b - a) * t;
}

template<typename T>
inline constexpr T Smoothstep(T edge0, T edge1, T x)
{
	T const t = Clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
	return t * t * (T(3) - T(2) * t);
}

template<typename T>
inline constexpr T Min(T a, T b)
{
	return std::min(a, b);
}

template<typename T>
inline constexpr T Max(T a, T b)
{
	return std::max(a, b);
}

template<typename T>
inline constexpr T Abs(T value)
{
	return std::abs(value);
}

// Vector-specific functions (using GLM)
inline float Distance(Vec3 const& a, Vec3 const& b)
{
	return glm::distance(a, b);
}

inline Vec3 Normalize(Vec3 const& v)
{
	return glm::normalize(v);
}

inline Vec4 Normalize(Vec4 const& v)
{
	return glm::normalize(v);
}

inline Quat Normalize(Quat const& q)
{
	return glm::normalize(q);
}

inline float Dot(Vec3 const& a, Vec3 const& b)
{
	return glm::dot(a, b);
}

inline float Length(Vec3 const& v)
{
	return glm::length(v);
}

inline float LengthSquared(Vec3 const& v)
{
	return Dot(v, v);
}

inline Vec3 Cross(Vec3 const& a, Vec3 const& b)
{
	return glm::cross(a, b);
}

inline Vec3 Reflect(Vec3 const& incident, Vec3 const& normal)
{
	return glm::reflect(incident, normal);
}

inline Vec3 Mix(Vec3 const& a, Vec3 const& b, float c)
{
	return glm::mix(a, b, c);
}

// Component-wise min/max for vectors
inline Vec3 Min(Vec3 const& a, Vec3 const& b)
{
	return glm::min(a, b);
}

inline Vec3 Max(Vec3 const& a, Vec3 const& b)
{
	return glm::max(a, b);
}

// Lexicographic comparison functor for Vec3 (for use with std::map)
struct Vec3LexicographicLess final
{
	bool operator()(Vec3 const& a, Vec3 const& b) const
	{
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		return a.z < b.z;
	}
};

// Matrix functions
inline Mat4 Inverse(Mat4 const& m)
{
	return glm::inverse(m);
}

inline Mat3 Inverse(Mat3 const& m)
{
	return glm::inverse(m);
}

inline Mat3 Transpose(Mat3 const& m)
{
	return glm::transpose(m);
}

inline Mat4 Transpose(Mat4 const& m)
{
	return glm::transpose(m);
}

template<typename T>
inline Mat4 MakeMat4(T const* const data)
{
	return glm::make_mat4(data);
}

// Create diagonal matrix (identity when diagonal = 1.0)
template<typename T>
inline Mat4 MakeMat4(T diagonal)
{
	return Mat4(diagonal);
}

template<typename T>
inline Mat3 MakeMat3(T diagonal)
{
	return Mat3(diagonal);
}

// Extract basis vectors from transformation matrix
inline Vec3 GetRightVector(Mat4 const& m)
{
	return Vec3(m[0][0], m[0][1], m[0][2]);
}

inline Vec3 GetUpVector(Mat4 const& m)
{
	return Vec3(m[1][0], m[1][1], m[1][2]);
}

inline Vec3 GetForwardVector(Mat4 const& m)
{
	return Vec3(m[2][0], m[2][1], m[2][2]);
}

inline Vec3 GetTranslation(Mat4 const& m)
{
	return Vec3(m[3][0], m[3][1], m[3][2]);
}
} // namespace Tge::Math
