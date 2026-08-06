#include <gtest/gtest.h>

#include <tge/math/constants.hpp>
#include <tge/math/functions.hpp>
#include <tge/math/transform.hpp>

using Tge::Math::CTransform;
using Tge::Math::Mat4;
using Tge::Math::Quat;
using Tge::Math::Vec3;

namespace Tge::Testing
{
using namespace Tge::Math;

namespace
{
// A mirror has no positive-scale TRS form, so a decomposition that reads scale as a basis length
// cannot round-trip it. Determinant sign is the property that must survive.
float BasisDeterminant(Mat4 const& matrix)
{
	Vec3 const basisX{ matrix[0] };
	Vec3 const basisY{ matrix[1] };
	Vec3 const basisZ{ matrix[2] };

	return Dot(Cross(basisX, basisY), basisZ);
}

void ExpectMatrixNear(Mat4 const& actual, Mat4 const& expected)
{
	for (int column = 0; column < 4; column++)
	{
		for (int row = 0; row < 4; row++)
		{
			EXPECT_NEAR(actual[column][row], expected[column][row], 1e-5f)
				<< "at column " << column << " row " << row;
		}
	}
}

// Decompose, then rebuild. The contract is that this is lossless.
void ExpectRoundTrip(Mat4 const& original)
{
	CTransform const decomposed = CTransform::FromMatrix(original);

	ExpectMatrixNear(decomposed.ToMatrix(), original);
	EXPECT_NEAR(BasisDeterminant(decomposed.ToMatrix()), BasisDeterminant(original), 1e-5f);
}
} // namespace

//////////////////////////////////////////////////////////////////////////
// NegativeScaleTest's `NegativeScaleFront` node: a 180 degree Y rotation composed with a uniform
// -1 scale, which nets out to a mirror in Y. Losing it renders the node's texture upside-down and
// leaves its winding unreversed.
TEST(TransformDecompose, MirroredNodeRoundTrips)
{
	Quat const rotation{ 0.0f, 0.0f, 1.0f, 0.0f }; // w, x, y, z -- 180 degrees about Y
	Mat4 const original = Translate(Vec3{ 0.007f, 1.52f, 0.1f })
	                    * QuaternionToMatrix(rotation)
	                    * Scale(Vec3{ -1.0f, -1.0f, -1.0f });

	ASSERT_LT(BasisDeterminant(original), 0.0f);

	ExpectRoundTrip(original);
}

//////////////////////////////////////////////////////////////////////////
TEST(TransformDecompose, UniformNegativeScaleRoundTrips)
{
	Mat4 const original = Scale(Vec3{ -1.0f, -1.0f, -1.0f });

	ASSERT_LT(BasisDeterminant(original), 0.0f);

	ExpectRoundTrip(original);
}

//////////////////////////////////////////////////////////////////////////
// A mirror on a single axis, with a non-uniform scale, so the sign cannot hide in a uniform factor.
TEST(TransformDecompose, SingleAxisMirrorRoundTrips)
{
	Mat4 const original = Translate(Vec3{ 3.0f, -2.0f, 5.0f })
	                    * QuaternionToMatrix(QuaternionFromAxisAngle(Vec3UnitZ, 0.75f))
	                    * Scale(Vec3{ 2.0f, -3.0f, 4.0f });

	ASSERT_LT(BasisDeterminant(original), 0.0f);

	ExpectRoundTrip(original);
}

//////////////////////////////////////////////////////////////////////////
// The common path must stay untouched by the mirror handling.
TEST(TransformDecompose, ProperRotationRoundTrips)
{
	Mat4 const original = Translate(Vec3{ -4.0f, 6.0f, 1.0f })
	                    * QuaternionToMatrix(QuaternionFromAxisAngle(Vec3UnitY, 1.1f))
	                    * Scale(Vec3{ 2.0f, 3.0f, 4.0f });

	ASSERT_GT(BasisDeterminant(original), 0.0f);

	ExpectRoundTrip(original);
}
} // namespace Tge::Testing
