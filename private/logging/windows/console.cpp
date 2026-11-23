#include "logging/console.hpp"

#include <array>
#include <optional>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

namespace Tge::Logging
{
namespace
{
constexpr std::array<DWORD, 2> ConsoleOutputs{ STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };

// The console is shared with whoever started the program, so what Initialize changes Terminate gives back.
UINT gSavedCodePage{ 0 };
std::array<std::optional<DWORD>, 2> gSavedModes{};
} // namespace

//////////////////////////////////////////////////////////////////////////
// Saved before anything is set, and only once: both handles are usually one console, and Initialize may run again.
void PrepareConsole()
{
	if (gSavedCodePage == 0)
	{
		gSavedCodePage = GetConsoleOutputCP();
	}

	for (size_t index{ 0 }; index < ConsoleOutputs.size(); ++index)
	{
		DWORD mode{ 0 };

		if (!gSavedModes[index].has_value() && GetConsoleMode(GetStdHandle(ConsoleOutputs[index]), &mode) != FALSE)
		{
			gSavedModes[index] = mode;
		}
	}

	if (gSavedCodePage != 0)
	{
		SetConsoleOutputCP(CP_UTF8);
	}

	for (size_t index{ 0 }; index < ConsoleOutputs.size(); ++index)
	{
		if (gSavedModes[index].has_value())
		{
			SetConsoleMode(GetStdHandle(ConsoleOutputs[index]), *gSavedModes[index] | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void RestoreConsole()
{
	if (gSavedCodePage != 0)
	{
		SetConsoleOutputCP(gSavedCodePage);
		gSavedCodePage = 0;
	}

	for (size_t index{ 0 }; index < ConsoleOutputs.size(); ++index)
	{
		if (gSavedModes[index].has_value())
		{
			SetConsoleMode(GetStdHandle(ConsoleOutputs[index]), *gSavedModes[index]);
			gSavedModes[index].reset();
		}
	}
}
} // namespace Tge::Logging
