#include <gtest/gtest.h>
#include <tge/math/types.hpp>
#include <tge/math/functions.hpp>
#include <tge/math/glm_include.hpp>

using namespace Tge::Math;

// Test that Mat3(Mat4) extracts the upper-left 3x3 correctly
TEST(NormalMatrixTest, Mat3FromMat4Extraction)
{
	// Create a test transform matrix with rotation and translation
	Mat4 model = MakeMat4(1.0f);
	model[0][0] = 2.0f; // Scale X
	model[1][1] = 3.0f; // Scale Y
	model[2][2] = 4.0f; // Scale Z
	model[3][0] = 10.0f; // Translation X
	model[3][1] = 20.0f; // Translation Y
	model[3][2] = 30.0f; // Translation Z

	// Extract 3x3
	Mat3 mat3 = Mat3(model);

	// Verify it extracted the upper-left 3x3 (rotation/scale part)
	EXPECT_FLOAT_EQ(mat3[0][0], 2.0f);
	EXPECT_FLOAT_EQ(mat3[1][1], 3.0f);
	EXPECT_FLOAT_EQ(mat3[2][2], 4.0f);

	// Verify translation was NOT included
	EXPECT_FLOAT_EQ(mat3[0][2], 0.0f);
	EXPECT_FLOAT_EQ(mat3[1][2], 0.0f);
	EXPECT_FLOAT_EQ(mat3[2][2], 4.0f); // This should be the scale, not translation
}

// Test normal matrix computation matches GLSL shader behavior
TEST(NormalMatrixTest, NormalMatrixComputation)
{
	// Create a simple non-uniform scale transform
	Mat4 model = MakeMat4(1.0f);
	model[0][0] = 2.0f; // Scale X by 2
	model[1][1] = 1.0f; // Scale Y by 1 (no scale)
	model[2][2] = 0.5f; // Scale Z by 0.5

	// Compute normal matrix: transpose(inverse(mat3(model)))
	Mat3 modelMatrix3x3 = Mat3(model);
	Mat3 normalMatrix = Transpose(Inverse(modelMatrix3x3));

	// For a diagonal scale matrix, inverse flips the scales
	// and transpose doesn't change diagonal matrices
	EXPECT_NEAR(normalMatrix[0][0], 1.0f / 2.0f, 0.0001f);   // 1/2 = 0.5
	EXPECT_NEAR(normalMatrix[1][1], 1.0f / 1.0f, 0.0001f);   // 1/1 = 1.0
	EXPECT_NEAR(normalMatrix[2][2], 1.0f / 0.5f, 0.0001f);   // 1/0.5 = 2.0

	// Off-diagonal should be zero
	EXPECT_NEAR(normalMatrix[0][1], 0.0f, 0.0001f);
	EXPECT_NEAR(normalMatrix[1][0], 0.0f, 0.0001f);
}

// Test that normal matrix correctly transforms normals
TEST(NormalMatrixTest, NormalTransformation)
{
	// Non-uniform scale that would distort normals
	Mat4 model = MakeMat4(1.0f);
	model[0][0] = 2.0f; // Scale X by 2
	model[1][1] = 1.0f; // No Y scale
	model[2][2] = 1.0f; // No Z scale

	Mat3 modelMatrix3x3 = Mat3(model);
	Mat3 normalMatrix = Transpose(Inverse(modelMatrix3x3));

	// A normal pointing along X axis
	Vec3 normal(1.0f, 0.0f, 0.0f);

	// Transform with normal matrix (CORRECT for normals)
	// Note: Using model matrix directly would be WRONG - it would stretch the normal
	Vec3 correctNormal = normalMatrix * normal;
	// correctNormal = (0.5, 0, 0) then normalize

	// The normal matrix should compensate for non-uniform scale
	EXPECT_NEAR(correctNormal.x, 0.5f, 0.0001f);
	EXPECT_NEAR(correctNormal.y, 0.0f, 0.0001f);
	EXPECT_NEAR(correctNormal.z, 0.0f, 0.0001f);

	// After normalization, it should point along X
	Vec3 normalized = Normalize(correctNormal);
	EXPECT_NEAR(normalized.x, 1.0f, 0.0001f);
	EXPECT_NEAR(normalized.y, 0.0f, 0.0001f);
	EXPECT_NEAR(normalized.z, 0.0f, 0.0001f);
}

// Test identity matrix case (no transformation)
TEST(NormalMatrixTest, IdentityMatrix)
{
	Mat4 model = MakeMat4(1.0f); // Identity

	Mat3 modelMatrix3x3 = Mat3(model);
	Mat3 normalMatrix = Transpose(Inverse(modelMatrix3x3));

	// Normal matrix of identity should be identity
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			float expected = (i == j) ? 1.0f : 0.0f;
			EXPECT_NEAR(normalMatrix[i][j], expected, 0.0001f);
		}
	}
}

// Test GLM column-major vs row-major indexing
TEST(NormalMatrixTest, GLMIndexing)
{
	// GLM uses column-major ordering like GLSL
	// mat[col][row] NOT mat[row][col]
	Mat3 m = MakeMat3(0.0f);
	m[0][0] = 1.0f; // Column 0, Row 0
	m[1][0] = 2.0f; // Column 1, Row 0
	m[2][0] = 3.0f; // Column 2, Row 0
	m[0][1] = 4.0f; // Column 0, Row 1

	// Access should match GLSL shader indexing
	EXPECT_FLOAT_EQ(m[0][0], 1.0f);
	EXPECT_FLOAT_EQ(m[1][0], 2.0f);
	EXPECT_FLOAT_EQ(m[2][0], 3.0f);
	EXPECT_FLOAT_EQ(m[0][1], 4.0f);

	// Verify it's column-major storage
	float const* data = &m[0][0];
	EXPECT_FLOAT_EQ(data[0], 1.0f); // m[0][0]
	EXPECT_FLOAT_EQ(data[1], 4.0f); // m[0][1]
	EXPECT_FLOAT_EQ(data[3], 2.0f); // m[1][0]
}
