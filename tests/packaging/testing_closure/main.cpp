#include <tge/logging/log.hpp>
#include <tge/testing/expected_log_errors.hpp>

#include <gtest/gtest.h>

namespace
{
Tge::Logging::CLog gLog{ "PackagingClosure" };
} // namespace

// Inside a test rather than in main: the harness EXPECTs the channel it mutes to exist, and outside a test
// body that expectation has nothing to report to. Constructing it is what pulls the header's own use of the
// log system through the link.
TEST(TestingClosure, SwallowsAnExpectedError)
{
	Tge::Testing::CExpectedLogErrors const expected{ "PackagingClosure" };

	gLog.Error("expected by the packaging gate, swallowed by the harness");
}

//////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
