#include <tge/assert.hpp>

#include <gtest/gtest.h>
#include <cstdint>

namespace
{
// The condition is the work, so a build that compiled it out, as TGE_ASSERT does in release, would skip the work itself.
// Negative control: define TGE_VERIFY as ((void)0) in the configuration under test, and the call never happens.
TEST(Verify, RunsItsConditionOnceInEveryBuild)
{
	uint32_t calls{ 0u };

	TGE_VERIFY(++calls == 1u, "the condition runs exactly once");

	EXPECT_EQ(calls, 1u);
}
} // namespace
