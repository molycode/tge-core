#include <tge/math/intersection.hpp>

#include <gtest/gtest.h>

namespace Tge::Testing
{
using namespace Tge::Math;

namespace
{
CAABB UnitBox()
{
	CAABB box{};
	box.min = Vec3{ 0.0f };
	box.max = Vec3{ 1.0f };

	return box;
}
} // namespace

//////////////////////////////////////////////////////////////////////////
// Under a metre a radius squared is smaller than the radius, over one larger, so a distance compared with it misses one
// side and invents the other.
// Negative control: the distance compared with the squared radius, and the box half a metre off is missed.
TEST(IntersectionTest, ASphereMeetsABoxWithinItsRadius)
{
	EXPECT_TRUE(Intersects(UnitBox(), SSphere{ Vec3{ 1.5f, 0.5f, 0.5f }, 0.6f }));
	EXPECT_FALSE(Intersects(UnitBox(), SSphere{ Vec3{ 1.5f, 0.5f, 0.5f }, 0.4f }));
	EXPECT_TRUE(Intersects(UnitBox(), SSphere{ Vec3{ 3.5f, 0.5f, 0.5f }, 2.6f }));
	EXPECT_FALSE(Intersects(UnitBox(), SSphere{ Vec3{ 3.5f, 0.5f, 0.5f }, 2.0f }));
}

//////////////////////////////////////////////////////////////////////////
// Negative control: the distance compared with the squared radius sum, and the spheres 0.7 m apart are missed.
TEST(IntersectionTest, TwoSpheresMeetWithinTheirRadii)
{
	EXPECT_TRUE(Intersects(SSphere{ Vec3{ 0.0f }, 0.4f }, SSphere{ Vec3{ 0.7f, 0.0f, 0.0f }, 0.4f }));
	EXPECT_FALSE(Intersects(SSphere{ Vec3{ 0.0f }, 0.3f }, SSphere{ Vec3{ 0.7f, 0.0f, 0.0f }, 0.3f }));
	EXPECT_TRUE(Intersects(SSphere{ Vec3{ 0.0f }, 2.6f }, SSphere{ Vec3{ 5.0f, 0.0f, 0.0f }, 2.6f }));
	EXPECT_FALSE(Intersects(SSphere{ Vec3{ 0.0f }, 2.0f }, SSphere{ Vec3{ 5.0f, 0.0f, 0.0f }, 2.0f }));
}
} // namespace Tge::Testing
