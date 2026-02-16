#include "loggers.hpp"

namespace Tge
{
Logging::CLog gLog{"Core"};

namespace Memory
{
Logging::CLog gLog{"Memory"};
} // namespace Memory

namespace IO
{
Logging::CLog gLog{"IO"};
} // namespace IO
} // namespace Tge
