#pragma once

namespace Tge
{
// Inherit from SNoCopy to disable copy operations while allowing move semantics
struct SNoCopy
{
	SNoCopy(SNoCopy const&) = delete;
	SNoCopy& operator=(SNoCopy const&) = delete;
	SNoCopy(SNoCopy&&) = default;
	SNoCopy& operator=(SNoCopy&&) = default;

protected:

	SNoCopy() = default;
	~SNoCopy() = default;
};

// Inherit from SNoMove to disable move operations while allowing copy semantics
struct SNoMove
{
	SNoMove(SNoMove const&) = default;
	SNoMove& operator=(SNoMove const&) = default;
	SNoMove(SNoMove&&) = delete;
	SNoMove& operator=(SNoMove&&) = delete;

protected:

	SNoMove() = default;
	~SNoMove() = default;
};

// Inherit from SNoCopyNoMove to disable both copy and move operations
// MSVC requires __declspec(empty_bases) to enable EBO for multiple empty base inheritance
#ifdef _MSC_VER
struct __declspec(empty_bases) SNoCopyNoMove : private SNoCopy, private SNoMove
#else
struct SNoCopyNoMove : private SNoCopy, private SNoMove
#endif
{
protected:

	SNoCopyNoMove() = default;
	~SNoCopyNoMove() = default;
};

// Compile-time verification: Ensure Empty Base Optimization works correctly
namespace Detail
{
struct TestEBO : SNoCopyNoMove
{
	int dummy; // 4 bytes
};

// Verify that inheriting from SNoCopyNoMove adds zero overhead
// If EBO fails, this will trigger a compile error with clear message
static_assert(sizeof(TestEBO) == sizeof(int),
              "Empty Base Optimization failed! SNoCopyNoMove should add zero size overhead. "
              "This indicates a compiler issue. Expected size: 4 bytes");
} // namespace Detail
} // namespace Tge
