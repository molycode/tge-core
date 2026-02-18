#pragma once

#include "types.hpp"

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

// Point containment tests
bool Contains(CAABB const& aabb, Vec3 const& point);
bool Contains(SSphere const& sphere, Vec3 const& point);
} // namespace Tge::Math
