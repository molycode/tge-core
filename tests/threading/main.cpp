#include <gtest/gtest.h>
#include <tge/init/init.hpp>

namespace
{
// Brings up the tge-core job scheduler (Memory -> Threading -> IO) once for the whole suite —
// CJobGroup jobs are no-ops without it. No renderer/window, so this runs headless anywhere.
class CThreadingTestEnvironment final : public ::testing::Environment
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
	::testing::AddGlobalTestEnvironment(new CThreadingTestEnvironment());
	return RUN_ALL_TESTS();
}
