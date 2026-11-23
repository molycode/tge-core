#include "logging/local_time.hpp"

namespace Tge::Logging
{
//////////////////////////////////////////////////////////////////////////
std::tm ToLocalTime(std::time_t const time)
{
	std::tm local{};
	localtime_r(&time, &local);

	return local;
}
} // namespace Tge::Logging
