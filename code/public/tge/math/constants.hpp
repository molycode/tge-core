#pragma once

#include "types.hpp"
#include <limits>
#include <numbers>
#include <concepts>

namespace Tge::Math
{
using TgeReal = float;

template<std::floating_point T>
struct SConstants final
{
	static inline constexpr T Pi        = std::numbers::pi_v<T>;
	static inline constexpr T TwoPi     = T{2.0} * Pi;
	static inline constexpr T HalfPi    = T{0.5} * Pi;
	static inline constexpr T QuarterPi = T{0.25} * Pi;
	static inline constexpr T InvPi     = T{1.0} / Pi;
	static inline constexpr T InvTwoPi  = T{1.0} / TwoPi;
	static inline constexpr T DegToRad  = Pi / T{180.0};
	static inline constexpr T RadToDeg  = T{180.0} / Pi;
	static inline constexpr T Epsilon   = std::numeric_limits<T>::epsilon();
	static inline constexpr T Infinity  = std::numeric_limits<T>::infinity();
};

inline constexpr auto Pi        = SConstants<TgeReal>::Pi;
inline constexpr auto TwoPi     = SConstants<TgeReal>::TwoPi;
inline constexpr auto HalfPi    = SConstants<TgeReal>::HalfPi;
inline constexpr auto QuarterPi = SConstants<TgeReal>::QuarterPi;
inline constexpr auto InvPi     = SConstants<TgeReal>::InvPi;
inline constexpr auto InvTwoPi  = SConstants<TgeReal>::InvTwoPi;
inline constexpr auto DegToRad  = SConstants<TgeReal>::DegToRad;
inline constexpr auto RadToDeg  = SConstants<TgeReal>::RadToDeg;
inline constexpr auto Epsilon   = SConstants<TgeReal>::Epsilon;
inline constexpr auto Infinity  = SConstants<TgeReal>::Infinity;

inline constexpr Vec2 Vec2Zero = Vec2{0.0f, 0.0f};
inline constexpr Vec2 Vec2One  = Vec2{1.0f, 1.0f};

inline constexpr Vec3 Vec3Zero    = Vec3{0.0f, 0.0f, 0.0f};
inline constexpr Vec3 Vec3One     = Vec3{1.0f, 1.0f, 1.0f};
inline constexpr Vec3 Vec3UnitX   = Vec3{1.0f, 0.0f, 0.0f};
inline constexpr Vec3 Vec3UnitY   = Vec3{0.0f, 1.0f, 0.0f};
inline constexpr Vec3 Vec3UnitZ   = Vec3{0.0f, 0.0f, 1.0f};
inline constexpr Vec3 Vec3Up      = Vec3UnitY;
inline constexpr Vec3 Vec3Down    = -Vec3UnitY;
inline constexpr Vec3 Vec3Right   = Vec3UnitX;
inline constexpr Vec3 Vec3Left    = -Vec3UnitX;
inline constexpr Vec3 Vec3Forward = Vec3UnitZ;
inline constexpr Vec3 Vec3Back    = -Vec3UnitZ;

inline constexpr Vec4 Vec4Zero = Vec4{0.0f, 0.0f, 0.0f, 0.0f};
inline constexpr Vec4 Vec4One  = Vec4{1.0f, 1.0f, 1.0f, 1.0f};

inline constexpr Mat4 IdentityMat4 = Mat4{1.0f};
inline constexpr Mat3 IdentityMat3 = Mat3{1.0f};

inline constexpr Quat IdentityQuat = Quat{1.0f, 0.0f, 0.0f, 0.0f};

// UV/Texture transform identity: scale=(1,1), offset=(0,0)
inline constexpr Vec4 UVTransformIdentity = Vec4{1.0f, 1.0f, 0.0f, 0.0f};
} // namespace Tge::Math
