#include <gtest/gtest.h>
#include <tge/io/get_user_folder.hpp>
#include <tge/platform.hpp>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <cstdlib>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

using namespace Tge::IO;

namespace
{
using Variables = std::vector<std::pair<char const*, std::optional<std::string>>>;

//////////////////////////////////////////////////////////////////////////
std::optional<std::string> GetVariable(char const* pName)
{
	std::optional<std::string> value{};

#if defined(TGE_PLATFORM_WINDOWS)
	std::wstring const name{ std::filesystem::path{ pName }.wstring() };
	DWORD const size{ GetEnvironmentVariableW(name.c_str(), nullptr, 0) };

	if (size > 0)
	{
		std::wstring wide{};

		wide.resize(size);
		wide.resize(GetEnvironmentVariableW(name.c_str(), wide.data(), size));
		value = std::filesystem::path{ wide }.string();
	}
#elif defined(TGE_PLATFORM_LINUX)
	char const* const pValue{ std::getenv(pName) };

	if (pValue != nullptr)
	{
		value = std::string{ pValue };
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return value;
}

//////////////////////////////////////////////////////////////////////////
void SetVariable(char const* pName, std::optional<std::string> const& value)
{
#if defined(TGE_PLATFORM_WINDOWS)
	std::wstring const name{ std::filesystem::path{ pName }.wstring() };
	std::wstring const wide{ value.has_value() ? std::filesystem::path{ *value }.wstring() : std::wstring{} };

	SetEnvironmentVariableW(name.c_str(), value.has_value() ? wide.c_str() : nullptr);
#elif defined(TGE_PLATFORM_LINUX)
	if (value.has_value())
	{
		setenv(pName, value->c_str(), 1);
	}
	else
	{
		unsetenv(pName);
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
// Asks with these variables set or unset, then puts each back as it was.
std::expected<std::filesystem::path, std::error_code> GetWith(EUserFolder folder, Variables const& variables)
{
	Variables previous{};

	for (auto const& [pName, value] : variables)
	{
		previous.emplace_back(pName, GetVariable(pName));
		SetVariable(pName, value);
	}

	std::expected<std::filesystem::path, std::error_code> const result{ GetUserFolder(folder) };

	for (auto const& [pName, value] : previous)
	{
		SetVariable(pName, value);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
Variables WithoutOverrides()
{
	return Variables{ { "XDG_CONFIG_HOME", std::nullopt }, { "XDG_DATA_HOME", std::nullopt }, { "XDG_STATE_HOME", std::nullopt },
		{ "XDG_CACHE_HOME", std::nullopt } };
}
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, AbsoluteOverrideWins)
{
	std::error_code error{};
	std::filesystem::path const overridden{ std::filesystem::temp_directory_path(error) / "tge-io-tests-cache" };

	ASSERT_EQ(error, std::error_code{});

	std::expected<std::filesystem::path, std::error_code> const folder{ GetWith(EUserFolder::Cache, { { "XDG_CACHE_HOME", overridden.string() } }) };

	ASSERT_TRUE(folder.has_value());
	EXPECT_EQ(*folder, overridden);
}

//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, RelativeOverrideIsIgnored)
{
	std::expected<std::filesystem::path, std::error_code> const relative{ GetWith(EUserFolder::Data,
		{ { "XDG_DATA_HOME", std::string{ "relative/data" } }, { "HOME", std::string{ "/home/tester" } } }) };
	std::expected<std::filesystem::path, std::error_code> const unset{ GetWith(EUserFolder::Data,
		{ { "XDG_DATA_HOME", std::nullopt }, { "HOME", std::string{ "/home/tester" } } }) };

	ASSERT_TRUE(relative.has_value());
	ASSERT_TRUE(unset.has_value());
	EXPECT_EQ(*relative, *unset);
	EXPECT_TRUE(relative->is_absolute());
}

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, RoamingForConfigAndDataLocalForStateAndCache)
{
	std::expected<std::filesystem::path, std::error_code> const config{ GetWith(EUserFolder::Config, WithoutOverrides()) };
	std::expected<std::filesystem::path, std::error_code> const data{ GetWith(EUserFolder::Data, WithoutOverrides()) };
	std::expected<std::filesystem::path, std::error_code> const state{ GetWith(EUserFolder::State, WithoutOverrides()) };
	std::expected<std::filesystem::path, std::error_code> const cache{ GetWith(EUserFolder::Cache, WithoutOverrides()) };
	std::error_code error{};

	ASSERT_TRUE(config.has_value() && data.has_value() && state.has_value() && cache.has_value());
	EXPECT_EQ(*config, *data);
	EXPECT_EQ(*state, *cache);
	EXPECT_NE(*config, *state);
	EXPECT_TRUE(std::filesystem::is_directory(*config, error));
	EXPECT_TRUE(std::filesystem::is_directory(*state, error));
}
#elif defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, HomeIsTheFallback)
{
	Variables variables{ WithoutOverrides() };

	variables.emplace_back("HOME", std::string{ "/home/tester" });

	EXPECT_EQ(GetWith(EUserFolder::Config, variables).value_or(std::filesystem::path{}), std::filesystem::path{ "/home/tester/.config" });
	EXPECT_EQ(GetWith(EUserFolder::Data, variables).value_or(std::filesystem::path{}), std::filesystem::path{ "/home/tester/.local/share" });
	EXPECT_EQ(GetWith(EUserFolder::State, variables).value_or(std::filesystem::path{}), std::filesystem::path{ "/home/tester/.local/state" });
	EXPECT_EQ(GetWith(EUserFolder::Cache, variables).value_or(std::filesystem::path{}), std::filesystem::path{ "/home/tester/.cache" });
}

//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, NoHomeIsAnError)
{
	std::expected<std::filesystem::path, std::error_code> const folder{ GetWith(EUserFolder::State, { { "XDG_STATE_HOME", std::nullopt }, { "HOME", std::nullopt } }) };

	ASSERT_FALSE(folder.has_value());
	EXPECT_EQ(folder.error().message(), "HOME is unset or not an absolute path");
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
