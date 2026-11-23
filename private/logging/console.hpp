#pragma once

namespace Tge::Logging
{
// Readies the terminal the log writes to for UTF-8 and colour sequences; RestoreConsole gives back what it changed.
void PrepareConsole();
void RestoreConsole();
} // namespace Tge::Logging
