#pragma once

#include "test_support.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <system_error>

namespace Tge::IO::Tests
{
class CReadWholeFileTest : public testing::Test
{
protected:

	// testing::Test
	void SetUp() override
	{
		m_folder = MakeTemporaryFolder("tge-io-read");
		ASSERT_FALSE(m_folder.empty());
	}

	void TearDown() override
	{
		std::error_code error{};

		std::filesystem::remove_all(m_folder, error);
	}
	// ~testing::Test

	std::filesystem::path m_folder{};
};
} // namespace Tge::IO::Tests
