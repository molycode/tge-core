#pragma once

#include "types.hpp"
#include "functions.hpp"
#include <array>

namespace Tge::Math
{
// Primitive shapes
struct SRay final
{
	Vec3 origin{0.0f};
	Vec3 direction{0.0f, 0.0f, 1.0f};
};

class CAABB final
{
public:

	Vec3 min{0.0f};
	Vec3 max{0.0f};

	Vec3 GetCenter() const { return (min + max) * 0.5f; }
	Vec3 GetExtents() const { return (max - min) * 0.5f; }
	Vec3 GetSize() const { return max - min; }
};

struct SSphere final
{
	Vec3 center{0.0f};
	float radius = 0.0f;
};

struct SPlane final
{
	Vec3 normal{0.0f, 1.0f, 0.0f};
	float distance = 0.0f;
};

struct SFrustum final
{
	std::array<SPlane, 6> planes;
};

// Gribb-Hartmann extraction; pass proj * view. Planes point inward and stay unnormalized, so a point is
// inside when n·p + d >= 0 against all six.
// Inline: callers test whole BVHs a node at a time and the consuming builds have no LTO.
inline SFrustum ExtractFrustum(Mat4 const& viewProj)
{
	Mat4 const& vp = viewProj;
	SFrustum frustum;

	// vp[col][row] — a transposed read yields six plausible but wrong planes.
	frustum.planes[0].normal   = { vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0] }; // left
	frustum.planes[0].distance =   vp[3][3] + vp[3][0];
	frustum.planes[1].normal   = { vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0] }; // right
	frustum.planes[1].distance =   vp[3][3] - vp[3][0];
	frustum.planes[2].normal   = { vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1] }; // bottom
	frustum.planes[2].distance =   vp[3][3] + vp[3][1];
	frustum.planes[3].normal   = { vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1] }; // top
	frustum.planes[3].distance =   vp[3][3] - vp[3][1];
	frustum.planes[4].normal   = { vp[0][2],             vp[1][2],             vp[2][2]             }; // near
	frustum.planes[4].distance =   vp[3][2];
	frustum.planes[5].normal   = { vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2] }; // far
	frustum.planes[5].distance =   vp[3][3] - vp[3][2];

	return frustum;
}

// Ray-AABB intersection (returns true if hit, outputs tMin and tMax)
bool Intersects(SRay const& ray, CAABB const& aabb, float& tMin, float& tMax);

// Ray-Sphere intersection (returns true if hit, outputs t)
bool Intersects(SRay const& ray, SSphere const& sphere, float& t);

// Ray-Plane intersection (returns true if hit, outputs t)
bool Intersects(SRay const& ray, SPlane const& plane, float& t);

// AABB-AABB intersection
bool Intersects(CAABB const& a, CAABB const& b);

// Sphere-Sphere intersection
bool Intersects(SSphere const& a, SSphere const& b);

// AABB-Sphere intersection
bool Intersects(CAABB const& aabb, SSphere const& sphere);

// AABB-Frustum intersection (positive-vertex test). Conservative — a large AABB straddling several
// planes can report a hit while lying outside, which is the accepted trade for culling.
inline bool Intersects(CAABB const& aabb, SFrustum const& frustum)
{
	bool intersects{ true };

	for (size_t i{ 0 }; i < frustum.planes.size() && intersects; ++i)
	{
		SPlane const& plane = frustum.planes[i];

		Vec3 const positiveVertex{
			plane.normal.x >= 0.0f ? aabb.max.x : aabb.min.x,
			plane.normal.y >= 0.0f ? aabb.max.y : aabb.min.y,
			plane.normal.z >= 0.0f ? aabb.max.z : aabb.min.z
		};

		intersects = Dot(plane.normal, positiveVertex) + plane.distance >= 0.0f;
	}

	return intersects;
}

// Point containment tests
bool Contains(CAABB const& aabb, Vec3 const& point);
bool Contains(SSphere const& sphere, Vec3 const& point);
} // namespace Tge::Math
