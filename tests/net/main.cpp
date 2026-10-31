#include <gtest/gtest.h>
#include <tge/net/socket_library.hpp>

namespace
{
class CNetTestEnvironment final : public ::testing::Environment
{
public:

	void SetUp() override
	{
		ASSERT_TRUE(m_library.Initialize().has_value());
	}

	void TearDown() override
	{
		m_library.Terminate();
	}

private:

	Tge::Net::CSocketLibrary m_library{};
};
} // namespace

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new CNetTestEnvironment());
	return RUN_ALL_TESTS();
}
