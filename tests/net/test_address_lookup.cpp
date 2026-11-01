#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/net/address_lookup.hpp>
#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>
#include <thread>

using namespace Tge::Net;
using namespace Tge::Net::Tests;

namespace
{
// .invalid is reserved never to resolve.
constexpr char const* MissingHost{ "lookout-tests.invalid" };

std::optional<std::expected<uint32_t, std::error_code>> Resolve(CAddressLookup& lookup)
{
	auto const deadline{ std::chrono::steady_clock::now() + Patience };
	std::optional<std::expected<uint32_t, std::error_code>> resolved{ lookup.Poll() };

	while (!resolved.has_value() && std::chrono::steady_clock::now() < deadline)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{ 5 });
		resolved = lookup.Poll();
	}

	return resolved;
}
} // namespace

class CAddressLookupTest : public testing::Test
{
protected:

	void SetUp() override
	{
#if defined(__SANITIZE_THREAD__)
		GTEST_SKIP() << "glibc starts getaddrinfo_a's thread outside TSan's hooks, and TSan crashes in it";
#endif // __SANITIZE_THREAD__
	}
};

//////////////////////////////////////////////////////////////////////////
TEST_F(CAddressLookupTest, ResolvesLocalhost)
{
	CAddressLookup lookup{};

	ASSERT_TRUE(lookup.Start("localhost").has_value());

	std::optional<std::expected<uint32_t, std::error_code>> const resolved{ Resolve(lookup) };

	ASSERT_TRUE(resolved.has_value());
	EXPECT_EQ(resolved->value_or(0), Loopback);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CAddressLookupTest, ReportsANameThatDoesNotResolve)
{
	CAddressLookup lookup{};

	ASSERT_TRUE(lookup.Start(MissingHost).has_value());

	std::optional<std::expected<uint32_t, std::error_code>> const resolved{ Resolve(lookup) };

	ASSERT_TRUE(resolved.has_value());
	ASSERT_FALSE(resolved->has_value());
	EXPECT_FALSE(resolved->error().message().empty());
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CAddressLookupTest, StartsAgainAfterTerminatingAPendingLookup)
{
	CAddressLookup lookup{};

	ASSERT_TRUE(lookup.Start(MissingHost).has_value());
	lookup.Terminate();
	ASSERT_TRUE(lookup.Start("localhost").has_value());

	std::optional<std::expected<uint32_t, std::error_code>> const resolved{ Resolve(lookup) };

	ASSERT_TRUE(resolved.has_value());
	EXPECT_EQ(resolved->value_or(0), Loopback);
}
