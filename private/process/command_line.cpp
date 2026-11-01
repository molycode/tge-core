#include "command_line.hpp"

namespace Tge::Process
{
namespace
{
constexpr std::string_view CharactersToQuote{ " \t\n\v\"" };

//////////////////////////////////////////////////////////////////////////
// The C runtime's rules, but only where needed: Quake-engine games split their command line on whitespace alone and
// keep quote characters, so a quoted "+connect" would reach them as a different word.
void AppendArgument(std::string& commandLine, std::string_view argument)
{
	if (!argument.empty() && argument.find_first_of(CharactersToQuote) == std::string_view::npos)
	{
		commandLine += argument;
	}
	else
	{
		size_t numBackslashes{ 0 };

		commandLine += '"';

		for (char const character : argument)
		{
			if (character == '\\')
			{
				++numBackslashes;
			}
			else if (character == '"')
			{
				commandLine.append(numBackslashes * 2 + 1, '\\');
				commandLine += '"';
				numBackslashes = 0;
			}
			else
			{
				commandLine.append(numBackslashes, '\\');
				commandLine += character;
				numBackslashes = 0;
			}
		}

		commandLine.append(numBackslashes * 2, '\\');
		commandLine += '"';
	}
}
} // namespace

//////////////////////////////////////////////////////////////////////////
// The runtime reads the program's name up to its closing quote, without escapes, and a path cannot hold a quote.
std::string BuildCommandLine(std::string_view program, std::span<std::string const> arguments)
{
	std::string commandLine{ "\"" };

	commandLine += program;
	commandLine += '"';

	for (std::string const& argument : arguments)
	{
		commandLine += ' ';
		AppendArgument(commandLine, argument);
	}

	return commandLine;
}
} // namespace Tge::Process
