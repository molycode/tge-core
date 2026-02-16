#include <tge/math/intersection.hpp>
#include <tge/math/functions.hpp>
#include <tge/math/constants.hpp>
#include <cmath>

namespace Tge::Math
{
//////////////////////////////////////////////////////////////////////////
bool Intersects(SRay const& ray, SAABB const& aabb, float& tMin, float& tMax)
{
	tMin = 0.0f;
	tMax = Infinity;

	for (int i = 0; i < 3; ++i)
	{
		if (std::abs(ray.direction[i]) > Epsilon)
		{
			float const t1 = (aabb.min[i] - ray.origin[i]) / ray.direction[i];
			float const t2 = (aabb.max[i] - ray.origin[i]) / ray.direction[i];

			tMin = Max(tMin, Min(t1, t2));
			tMax = Min(tMax, Max(t1, t2));

			if (tMax < tMin)
			{
				return false;
			}
		}
		else
		{
			if (ray.origin[i] < aabb.min[i] || ray.origin[i] > aabb.max[i])
			{
				return false;
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool Intersects(SRay const& ray, SSphere const& sphere, float& t)
{
	Vec3 const oc = ray.origin - sphere.center;
	float const a = Dot(ray.direction, ray.direction);
	float const b = 2.0f * Dot(oc, ray.direction);
	float const c = Dot(oc, oc) - sphere.radius * sphere.radius;
	float const discriminant = b * b - 4.0f * a * c;

	if (discriminant < 0.0f)
	{
		return false;
	}

	float const sqrtDiscriminant = std::sqrt(discriminant);
	float const t0 = (-b - sqrtDiscriminant) / (2.0f * a);
	float const t1 = (-b + sqrtDiscriminant) / (2.0f * a);

	if (t0 > Epsilon)
	{
		t = t0;
		return true;
	}

	if (t1 > Epsilon)
	{
		t = t1;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool Intersects(SRay const& ray, SPlane const& plane, float& t)
{
	float const denom = Dot(ray.direction, plane.normal);

	if (std::abs(denom) > Epsilon)
	{
		t = -(Dot(ray.origin, plane.normal) + plane.distance) / denom;
		return t >= 0.0f;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool Intersects(SAABB const& a, SAABB const& b)
{
	if (a.max.x < b.min.x || a.min.x > b.max.x)
	{
		return false;
	}

	if (a.max.y < b.min.y || a.min.y > b.max.y)
	{
		return false;
	}

	if (a.max.z < b.min.z || a.min.z > b.max.z)
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool Intersects(SSphere const& a, SSphere const& b)
{
	float const distanceSquared = Distance(a.center, b.center);
	float const radiusSum = a.radius + b.radius;
	return distanceSquared <= radiusSum * radiusSum;
}

//////////////////////////////////////////////////////////////////////////
bool Intersects(SAABB const& aabb, SSphere const& sphere)
{
	Vec3 const closestPoint = Math::Clamp(sphere.center, aabb.min, aabb.max);
	float const distanceSquared = Distance(sphere.center, closestPoint);
	return distanceSquared <= sphere.radius * sphere.radius;
}

//////////////////////////////////////////////////////////////////////////
bool Contains(SAABB const& aabb, Vec3 const& point)
{
	if (point.x < aabb.min.x || point.x > aabb.max.x)
	{
		return false;
	}

	if (point.y < aabb.min.y || point.y > aabb.max.y)
	{
		return false;
	}

	if (point.z < aabb.min.z || point.z > aabb.max.z)
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool Contains(SSphere const& sphere, Vec3 const& point)
{
	float const distanceSquared = Distance(sphere.center, point);
	return distanceSquared <= sphere.radius * sphere.radius;
}
} // namespace Tge::Math
