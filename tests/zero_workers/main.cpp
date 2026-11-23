#include <gtest/gtest.h>
#include <tge/init/init.hpp>
#include <tge/logging/log_system.hpp>

namespace
{
// A suite of its own: the threading suite needs a pool with workers, these need one without any.
class CZeroWorkerEnvironment final : public ::testing::Environment
{
public:

	void SetUp() override
	{
		// Empty logs/config directories keep the suite from writing a file or reading the shipped channel levels.
		Tge::Logging::GetLogSystem().Initialize("test", "", "", Tge::Logging::ETimestampMode::Elapsed);
		Tge::Initialize(0u);
	}

	void TearDown() override
	{
		Tge::Terminate();
		Tge::Logging::GetLogSystem().Terminate();
	}
};
} // namespace

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new CZeroWorkerEnvironment());
	return RUN_ALL_TESTS();
}
