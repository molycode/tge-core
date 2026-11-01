#pragma once

#include <cstdint>
#include <string>

namespace Tge::Process
{
struct SExitStatus final
{
	uint32_t code{ 0 };
	std::string signal{};
};
} // namespace Tge::Process
