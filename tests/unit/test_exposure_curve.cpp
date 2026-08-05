#include <gtest/gtest.h>
#include <tge/exposure/exposure_curve.hpp>

#include <cmath>
#include <limits>

using Tge::Exposure::CExposureCurve;
using Tge::Exposure::MaxExposureCurveKeys;
using Tge::Exposure::SExposureCurveKey;

//////////////////////////////////////////////////////////////////////////
// An EXACT zero, not an approximate one: the meter adds this to its compensation, and only a literal 0.0f leaves that sum bit-for-bit unchanged.
TEST(CExposureCurveTest, EmptyCurveIsZeroStops)
{
	CExposureCurve const curve;

	EXPECT_EQ(curve.GetNumKeys(), 0u);
	EXPECT_FLOAT_EQ(curve.Evaluate(-100.0f), 0.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(15.0f), 0.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(100.0f), 0.0f);
}

//////////////////////////////////////////////////////////////////////////
// One key is the shape a segment search most easily gets wrong: finding no segment and falling through to zero.
TEST(CExposureCurveTest, SingleKeyIsConstantEverywhere)
{
	CExposureCurve curve;
	ASSERT_TRUE(curve.AddKey(10.0f, -2.0f));

	EXPECT_FLOAT_EQ(curve.Evaluate(-100.0f), -2.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(10.0f), -2.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(100.0f), -2.0f);
}

//////////////////////////////////////////////////////////////////////////
// An off-by-one in the segment search is invisible between keys but exact at them.
TEST(CExposureCurveTest, EvaluatesExactlyAtKeys)
{
	CExposureCurve curve;
	ASSERT_TRUE(curve.AddKey(0.0f, 0.0f));
	ASSERT_TRUE(curve.AddKey(10.0f, -4.0f));
	ASSERT_TRUE(curve.AddKey(20.0f, 1.0f));

	EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(10.0f), -4.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(20.0f), 1.0f);
}

//////////////////////////////////////////////////////////////////////////
TEST(CExposureCurveTest, InterpolatesLinearlyBetweenKeys)
{
	CExposureCurve curve;
	ASSERT_TRUE(curve.AddKey(0.0f, 0.0f));
	ASSERT_TRUE(curve.AddKey(10.0f, -4.0f));

	EXPECT_FLOAT_EQ(curve.Evaluate(5.0f), -2.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(2.5f), -1.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(7.5f), -3.0f);
}

//////////////////////////////////////////////////////////////////////////
// Extrapolating the end slope would keep darkening a scene already past the darkest EV anyone authored for — unbounded, and from data the artist never wrote.
TEST(CExposureCurveTest, ClampsBeyondTheEndKeys)
{
	CExposureCurve curve;
	ASSERT_TRUE(curve.AddKey(0.0f, 0.0f));
	ASSERT_TRUE(curve.AddKey(10.0f, -4.0f));

	EXPECT_FLOAT_EQ(curve.Evaluate(-5.0f), 0.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(-1000.0f), 0.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(15.0f), -4.0f);
	EXPECT_FLOAT_EQ(curve.Evaluate(1000.0f), -4.0f);
}

//////////////////////////////////////////////////////////////////////////
// Evaluate assumes sorted keys, and the console hands them over in whatever order they are typed.
TEST(CExposureCurveTest, KeysAreKeptSortedRegardlessOfInsertionOrder)
{
	CExposureCurve curve;
	ASSERT_TRUE(curve.AddKey(10.0f, -4.0f));
	ASSERT_TRUE(curve.AddKey(0.0f, 0.0f));
	ASSERT_TRUE(curve.AddKey(5.0f, -1.0f));

	ASSERT_EQ(curve.GetNumKeys(), 3u);
	EXPECT_FLOAT_EQ(curve.GetKey(0u).ev100, 0.0f);
	EXPECT_FLOAT_EQ(curve.GetKey(1u).ev100, 5.0f);
	EXPECT_FLOAT_EQ(curve.GetKey(2u).ev100, 10.0f);

	// The middle key is only reachable through a correctly ordered array.
	EXPECT_FLOAT_EQ(curve.Evaluate(5.0f), -1.0f);
}

//////////////////////////////////////////////////////////////////////////
// Two keys at one EV are a zero divisor in Evaluate, and the NaN would reach the meter's accumulator and stay there for the rest of the process.
TEST(CExposureCurveTest, DuplicateEvReplacesRatherThanDividingByZero)
{
	CExposureCurve curve;
	ASSERT_TRUE(curve.AddKey(0.0f, 0.0f));
	ASSERT_TRUE(curve.AddKey(10.0f, -4.0f));
	ASSERT_TRUE(curve.AddKey(10.0f, -1.0f));

	EXPECT_EQ(curve.GetNumKeys(), 2u);
	EXPECT_FLOAT_EQ(curve.Evaluate(10.0f), -1.0f);
	EXPECT_TRUE(std::isfinite(curve.Evaluate(10.0f)));
	EXPECT_TRUE(std::isfinite(curve.Evaluate(5.0f)));
}

//////////////////////////////////////////////////////////////////////////
// std::from_chars parses "inf" and "nan" and the scene loader's is_number() admits infinity, so a non-finite key is reachable from ordinary input.
TEST(CExposureCurveTest, NonFiniteKeysAreRejected)
{
	CExposureCurve curve;
	float const infinity = std::numeric_limits<float>::infinity();
	float const notANumber = std::numeric_limits<float>::quiet_NaN();

	EXPECT_FALSE(curve.AddKey(notANumber, 0.0f));
	EXPECT_FALSE(curve.AddKey(0.0f, notANumber));
	EXPECT_FALSE(curve.AddKey(infinity, 0.0f));
	EXPECT_FALSE(curve.AddKey(0.0f, infinity));
	EXPECT_FALSE(curve.AddKey(-infinity, 0.0f));

	EXPECT_EQ(curve.GetNumKeys(), 0u);
	EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 0.0f);
}

//////////////////////////////////////////////////////////////////////////
TEST(CExposureCurveTest, KeysBeyondCapacityAreRefused)
{
	CExposureCurve curve;

	for (uint32_t key = 0u; key < MaxExposureCurveKeys; ++key)
	{
		EXPECT_TRUE(curve.AddKey(static_cast<float>(key), -1.0f)) << "at key " << key;
	}

	EXPECT_EQ(curve.GetNumKeys(), MaxExposureCurveKeys);
	EXPECT_FALSE(curve.AddKey(1000.0f, -2.0f));
	EXPECT_EQ(curve.GetNumKeys(), MaxExposureCurveKeys);

	// A refused key must not have displaced an accepted one either.
	EXPECT_FLOAT_EQ(curve.Evaluate(1000.0f), -1.0f);
}

//////////////////////////////////////////////////////////////////////////
TEST(CExposureCurveTest, RemovingAKeyClosesTheGap)
{
	CExposureCurve curve;
	ASSERT_TRUE(curve.AddKey(0.0f, 0.0f));
	ASSERT_TRUE(curve.AddKey(5.0f, -1.0f));
	ASSERT_TRUE(curve.AddKey(10.0f, -4.0f));

	EXPECT_TRUE(curve.RemoveKey(1u));
	ASSERT_EQ(curve.GetNumKeys(), 2u);
	EXPECT_FLOAT_EQ(curve.GetKey(1u).ev100, 10.0f);

	// With the middle key gone the segment spans the whole range, so the midpoint moves.
	EXPECT_FLOAT_EQ(curve.Evaluate(5.0f), -2.0f);

	EXPECT_FALSE(curve.RemoveKey(2u));
}
