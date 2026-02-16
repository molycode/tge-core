#pragma once

#include "types.hpp"

namespace Tge::Math
{
struct STransform final
{
	Vec3 position{0.0f};
	Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // Identity quaternion (w, x, y, z)
	Vec3 scale{1.0f};

	Mat4 ToMatrix() const;
	static STransform FromMatrix(Mat4 const& matrix);
};

// Matrix construction functions
inline Mat4 Translate(Vec3 const& translation)
{
	return glm::translate(glm::mat4(1.0f), translation);
}

inline Mat4 Rotate(float angle, Vec3 const& axis)
{
	return glm::rotate(glm::mat4(1.0f), angle, axis);
}

inline Mat4 Scale(Vec3 const& scale)
{
	return glm::scale(glm::mat4(1.0f), scale);
}

inline Mat4 LookAt(Vec3 const& eye, Vec3 const& center, Vec3 const& up)
{
	return glm::lookAt(eye, center, up);
}

inline Mat4 Perspective(float fovy, float aspect, float near, float far)
{
	return glm::perspective(fovy, aspect, near, far);
}

inline Mat4 Orthographic(float left, float right, float bottom, float top, float near, float far)
{
	return glm::ortho(left, right, bottom, top, near, far);
}

// Quaternion functions
inline Quat QuaternionFromAxisAngle(Vec3 const& axis, float angle)
{
	return glm::angleAxis(angle, axis);
}

inline Quat QuaternionFromEuler(Vec3 const& eulerAngles)
{
	return Quat(eulerAngles);
}

inline Vec3 QuaternionToEuler(Quat const& q)
{
	return glm::eulerAngles(q);
}

inline Mat4 QuaternionToMatrix(Quat const& q)
{
	return glm::toMat4(q);
}

inline Quat QuaternionFromMatrix(Mat3 const& m)
{
	return glm::quat_cast(m);
}

inline Quat QuaternionSlerp(Quat const& a, Quat const& b, float t)
{
	return glm::slerp(a, b, t);
}
} // namespace Tge::Math
