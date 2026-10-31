#include <gtest/gtest.h>
#include <tge/init/init.hpp>

namespace
{
// The folder watch tests run on an event loop, whose thread needs tge-core brought up.
class CIOTestEnvironment final : public ::testing::Environment
{
public:

	void SetUp() override
	{
		Tge::Initialize();
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
	::testing::AddGlobalTestEnvironment(new CIOTestEnvironment());
	return RUN_ALL_TESTS();
}
