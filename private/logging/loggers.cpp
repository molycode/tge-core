#include <tge/logging/loggers.hpp>

namespace Tge
{
Logging::CLog gLog{"Tge"};

namespace Memory
{
Logging::CLog gLog{"Memory"};
} // namespace Memory

namespace Threading
{
Logging::CLog gLog{"Threading"};
} // namespace Threading

namespace IO
{
Logging::CLog gLog{"IO"};
} // namespace IO

namespace Command
{
Logging::CLog gLog{"Command"};
} // namespace Command
} // namespace Tge
