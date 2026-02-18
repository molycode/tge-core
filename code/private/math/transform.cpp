#include <tge/math/transform.hpp>
#include <tge/math/functions.hpp>

namespace Tge::Math
{
//////////////////////////////////////////////////////////////////////////
Mat4 CTransform::ToMatrix() const
{
	Mat4 const translationMatrix = Translate(position);
	Mat4 const rotationMatrix = QuaternionToMatrix(rotation);
	Mat4 const scaleMatrix = Scale(scale);

	return translationMatrix * rotationMatrix * scaleMatrix;
}

//////////////////////////////////////////////////////////////////////////
CTransform CTransform::FromMatrix(Mat4 const& matrix)
{
	CTransform transform;

	// Extract translation (last column)
	transform.position = Vec3(matrix[3]);

	// Extract scale (length of each basis vector)
	Vec3 const scale_x(matrix[0]);
	Vec3 const scale_y(matrix[1]);
	Vec3 const scale_z(matrix[2]);

	transform.scale = Vec3(
		Math::Length(scale_x),
		Math::Length(scale_y),
		Math::Length(scale_z)
	);

	// Extract rotation (normalize basis vectors and construct rotation matrix)
	Mat3 rotationMatrix{1.0f};

	if (transform.scale.x > 0.0f)
	{
		rotationMatrix[0] = scale_x / transform.scale.x;
	}

	if (transform.scale.y > 0.0f)
	{
		rotationMatrix[1] = scale_y / transform.scale.y;
	}

	if (transform.scale.z > 0.0f)
	{
		rotationMatrix[2] = scale_z / transform.scale.z;
	}

	transform.rotation = QuaternionFromMatrix(rotationMatrix);

	return transform;
}
} // namespace Tge::Math
