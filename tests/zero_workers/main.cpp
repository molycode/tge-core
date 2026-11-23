#include <gtest/gtest.h>
#include <tge/init/init.hpp>

namespace
{
// A suite of its own: the threading suite needs a pool with workers, these need one without any.
class CZeroWorkerEnvironment final : public ::testing::Environment
{
public:

	void SetUp() override
	{
		Tge::Initialize(0u);
	}

	void TearDown() override
	{
		Tge::Terminate();
	}
};
} // namespace

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new CZeroWorkerEnvironment());
	return RUN_ALL_TESTS();
}
