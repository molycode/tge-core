#include <gtest/gtest.h>
#include <tge/net/parse_ipv4.hpp>
#include <cstdint>
#include <optional>
#include <string_view>

using Tge::Net::ParseIpv4;

//////////////////////////////////////////////////////////////////////////
TEST(ParseIpv4, ReadsDottedDecimal)
{
	EXPECT_EQ(ParseIpv4("127.0.0.1"), std::optional<uint32_t>{ 0x7F000001 });
	EXPECT_EQ(ParseIpv4("0.0.0.0"), std::optional<uint32_t>{ 0 });
	EXPECT_EQ(ParseIpv4("255.255.255.255"), std::optional<uint32_t>{ 0xFFFFFFFF });
	EXPECT_EQ(ParseIpv4("10.20.30.40"), std::optional<uint32_t>{ 0x0A141E28 });
}

//////////////////////////////////////////////////////////////////////////
TEST(ParseIpv4, RejectsWhatInetPtonRejects)
{
	for (std::string_view const text : { "", "1.2.3", "1.2.3.4.5", "256.0.0.1", "01.2.3.4", "1.2.3.00", "1..2.3", ".1.2.3",
		"1.2.3.4.", " 1.2.3.4", "1.2.3.4 ", "a.b.c.d", "1.2.3.-4", "+1.2.3.4", "0x7f.0.0.1", "example.com", "1.2.3.4:80" })
	{
		EXPECT_FALSE(ParseIpv4(text).has_value()) << text;
	}
}
