#include <gtest/gtest.h>
#include <tge/testing/expected_log.hpp>
#include <tge/threading/parallel_for.hpp>

#include <algorithm>
#include <vector>

using namespace Tge::Threading;

//////////////////////////////////////////////////////////////////////////
TEST(ZeroWorkers, ParallelForRunsEveryIndexWithoutAWarning)
{
	Tge::Testing::CExpectedLog const expected{ "Threading", 0u, 0u };
	std::vector<uint32_t> visits(256u, 0u);

	ParallelFor(0u, static_cast<uint32_t>(visits.size()), [&visits](uint32_t index) { visits[index]++; }, 1u);

	EXPECT_EQ(std::ranges::count(visits, 1u), 256);
}
