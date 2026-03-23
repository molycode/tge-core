#include <tge/logging/log_system.hpp>
#include <tge/logging/log.hpp>
#include <tge/color.hpp>

#ifdef TGE_LOGGING_ENABLED
#include <algorithm>
#include <cassert>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <vector>
#endif // TGE_LOGGING_ENABLED

namespace Tge::Logging
{
#ifdef TGE_LOGGING_ENABLED
namespace
{
struct SChannelData final
{
	std::string name;
	SColor color;
	ELogLevel levelMask = ELogLevel::All;
};

struct SListener final
{
	void* key;
	LogMessageCallback callback;
};

constexpr size_t MaxMessages = 1024;

// Construct-on-first-use idiom to avoid static initialization order fiasco.
// CLog instances are global statics that register during static init,
// so these containers must be available before main() runs.

//////////////////////////////////////////////////////////////////////////
std::unordered_map<uint64_t, SChannelData>& GetChannels()
{
	static std::unordered_map<uint64_t, SChannelData> channels;
	return channels;
}

//////////////////////////////////////////////////////////////////////////
std::deque<SLogMessage>& GetMessages()
{
	static std::deque<SLogMessage> messages;
	return messages;
}

//////////////////////////////////////////////////////////////////////////
std::vector<SListener>& GetListeners()
{
	static std::vector<SListener> listeners;
	return listeners;
}

//////////////////////////////////////////////////////////////////////////
std::ofstream& GetLogFile()
{
	static std::ofstream logFile;
	return logFile;
}

//////////////////////////////////////////////////////////////////////////
std::mutex& GetMutex()
{
	static std::mutex mutex;
	return mutex;
}

//////////////////////////////////////////////////////////////////////////
std::chrono::system_clock::time_point& GetStartTime()
{
	static std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
	return startTime;
}

//////////////////////////////////////////////////////////////////////////
uint64_t HashChannelName(std::string_view name)
{
	return std::hash<std::string_view>{}(name);
}

//////////////////////////////////////////////////////////////////////////
bool ParseLogLevel(std::string_view str, ELogLevel& outLevel)
{
	// Parse compact bitmask: e=error, w=warning, i=info, a=all, n=none
	outLevel = ELogLevel::None;
	bool valid = !str.empty();

	for (char c : str)
	{
		if (valid)
		{
			switch (c)
			{
				case 'n': case 'N': outLevel = ELogLevel::None; break;
				case 'e': case 'E': outLevel = outLevel | ELogLevel::Error; break;
				case 'w': case 'W': outLevel = outLevel | ELogLevel::Warning; break;
				case 'i': case 'I': outLevel = outLevel | ELogLevel::Info; break;
				case 'a': case 'A': outLevel = ELogLevel::All; break;
				default:
					valid = false;
					break;
			}
		}
	}

	return valid;
}

//////////////////////////////////////////////////////////////////////////
std::string_view Trim(std::string_view str)
{
	size_t start = 0;
	size_t end = str.size();

	while (start < end && (str[start] == ' ' || str[start] == '\t'))
	{
		++start;
	}

	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t'))
	{
		--end;
	}

	return str.substr(start, end - start);
}

//////////////////////////////////////////////////////////////////////////
void LoadConfig(std::string_view configPath)
{
	std::ifstream file{std::string{configPath}};

	if (file.is_open())
	{
		ELogLevel defaultLevel = ELogLevel::All;
		bool hasDefault = false;
		std::vector<std::pair<std::string, ELogLevel>> channelOverrides;

		std::string line;

		while (std::getline(file, line))
		{
			std::string_view lineView = Trim(line);
			bool const isContent = !lineView.empty() && lineView[0] != '#';

			if (isContent)
			{
				size_t const eqPos = lineView.find('=');

				if (eqPos != std::string_view::npos)
				{
					std::string_view key = Trim(lineView.substr(0, eqPos));
					std::string_view value = Trim(lineView.substr(eqPos + 1));

					ELogLevel level;

					if (ParseLogLevel(value, level))
					{
						if (key == "default")
						{
							defaultLevel = level;
							hasDefault = true;
						}
						else
						{
							channelOverrides.emplace_back(std::string{key}, level);
						}
					}
				}
			}
		}

		// Apply default first
		if (hasDefault)
		{
			for (auto& [id, channel] : GetChannels())
			{
				channel.levelMask = defaultLevel;
			}
		}

		// Apply channel-specific overrides
		for (auto const& [name, level] : channelOverrides)
		{
			uint64_t const id = HashChannelName(name);
			auto it = GetChannels().find(id);

			if (it != GetChannels().end())
			{
				it->second.levelMask = level;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
std::string_view LevelToString(ELogLevel level)
{
	switch (level)
	{
		case ELogLevel::Error:   return "ERROR";
		case ELogLevel::Warning: return "WARNING";
		case ELogLevel::Info:    return "INFO";
		case ELogLevel::None:
		default:
			assert(false && "Invalid log level");
			return "UNKNOWN";
	}
}

//////////////////////////////////////////////////////////////////////////
void FormatTimestamp(uint64_t elapsedMs, char* buffer, size_t bufferSize)
{
	uint64_t const ms = elapsedMs % 1000;
	uint64_t const totalSec = elapsedMs / 1000;
	uint64_t const sec = totalSec % 60;
	uint64_t const totalMin = totalSec / 60;
	uint64_t const min = totalMin % 60;
	uint64_t const totalHours = totalMin / 60;
	uint64_t const hours = totalHours % 24;
	uint64_t const days = totalHours / 24;

	// Progressive format: show only what's needed
	// SS.mmm -> MM:SS.mmm -> HH:MM:SS.mmm -> D:HH:MM:SS.mmm
	if (days > 0)
	{
		std::snprintf(buffer, bufferSize, "%llu:%02llu:%02llu:%02llu.%03llu",
			static_cast<unsigned long long>(days),
			static_cast<unsigned long long>(hours),
			static_cast<unsigned long long>(min),
			static_cast<unsigned long long>(sec),
			static_cast<unsigned long long>(ms));
	}
	else if (hours > 0)
	{
		std::snprintf(buffer, bufferSize, "%02llu:%02llu:%02llu.%03llu",
			static_cast<unsigned long long>(hours),
			static_cast<unsigned long long>(min),
			static_cast<unsigned long long>(sec),
			static_cast<unsigned long long>(ms));
	}
	else if (min > 0)
	{
		std::snprintf(buffer, bufferSize, "%02llu:%02llu.%03llu",
			static_cast<unsigned long long>(min),
			static_cast<unsigned long long>(sec),
			static_cast<unsigned long long>(ms));
	}
	else
	{
		std::snprintf(buffer, bufferSize, "%02llu.%03llu",
			static_cast<unsigned long long>(sec),
			static_cast<unsigned long long>(ms));
	}
}

//////////////////////////////////////////////////////////////////////////
std::string FormatMessageForTerminal(SLogMessage const& msg)
{
	char timestamp[32];
	FormatTimestamp(msg.elapsedMs, timestamp, sizeof(timestamp));

	char buffer[512];

	// Terminal uses colors for severity, so no level prefix needed for INFO
	// WARNING and ERROR get prefix for extra visibility
	if (msg.level == ELogLevel::Info)
	{
		std::snprintf(buffer, sizeof(buffer), "[%s] [%s] %s",
			timestamp, msg.channelName.c_str(), msg.message.c_str());
	}
	else
	{
		std::snprintf(buffer, sizeof(buffer), "[%s] [%s] [%s] %s",
			timestamp, LevelToString(msg.level).data(),
			msg.channelName.c_str(), msg.message.c_str());
	}

	return buffer;
}

//////////////////////////////////////////////////////////////////////////
std::string FormatMessageForFile(SLogMessage const& msg)
{
	char timestamp[32];
	FormatTimestamp(msg.elapsedMs, timestamp, sizeof(timestamp));

	char buffer[512];

	// File has no colors, so always include level prefix
	std::snprintf(buffer, sizeof(buffer), "[%s] [%s] [%s] %s",
		timestamp, LevelToString(msg.level).data(),
		msg.channelName.c_str(), msg.message.c_str());

	return buffer;
}

//////////////////////////////////////////////////////////////////////////
void WriteToTerminal(SLogMessage const& msg)
{
	std::string formatted = FormatMessageForTerminal(msg);

	switch (msg.level)
	{
		case ELogLevel::Info:
		{
			// Use channel color if not white (default)
			bool const hasCustomColor = msg.colorR != 255 || msg.colorG != 255 || msg.colorB != 255;

			if (hasCustomColor)
			{
				// ANSI 24-bit color: \033[38;2;R;G;Bm
				std::cout << "\033[38;2;" << static_cast<int>(msg.colorR) << ";"
				          << static_cast<int>(msg.colorG) << ";"
				          << static_cast<int>(msg.colorB) << "m"
				          << formatted << "\033[0m\n" << std::flush;
			}
			else
			{
				std::cout << formatted << '\n' << std::flush;
			}
			break;
		}
		case ELogLevel::Warning:
			std::cout << "\033[33m" << formatted << "\033[0m\n" << std::flush;  // Yellow
			break;
		case ELogLevel::Error:
			std::cerr << "\033[31m" << formatted << "\033[0m\n" << std::flush;  // Red
			break;
		case ELogLevel::None:
		default:
			assert(false && "Invalid log level for message");
			break;
	}
}

//////////////////////////////////////////////////////////////////////////
void WriteToFile(SLogMessage const& msg)
{
	if (GetLogFile().is_open())
	{
		std::string formatted = FormatMessageForFile(msg);
		GetLogFile() << formatted << '\n';
		GetLogFile().flush();
	}
}

//////////////////////////////////////////////////////////////////////////
void NotifyListeners(SLogMessage const& msg)
{
	for (auto const& listener : GetListeners())
	{
		listener.callback(msg);
	}
}
} // namespace

//////////////////////////////////////////////////////////////////////////
void CLogSystem::Initialize(std::string_view prefix, std::string_view logsDir, std::string_view configDir)
{
	std::lock_guard lock(GetMutex());

	// Prime start time now so timestamps are relative to app startup, not first log message
	GetStartTime();

	if (!configDir.empty())
	{
		std::string configPath{ std::string(configDir) + "/logging.cfg" };
		LoadConfig(configPath);
	}

	if (!logsDir.empty())
	{
		std::filesystem::create_directories(logsDir);

		auto now = std::chrono::system_clock::now();
		auto time_t = std::chrono::system_clock::to_time_t(now);
		std::tm tm;
#ifdef _WIN32
		localtime_s(&tm, &time_t);
#else
		localtime_r(&time_t, &tm);
#endif

		std::string const format = std::string(logsDir) + "/" + std::string(prefix) + "_%Y-%m-%d_%H-%M-%S.log";
		char filename[256];
		std::strftime(filename, sizeof(filename), format.c_str(), &tm);

		GetLogFile().open(filename, std::ios::out | std::ios::app);
	}

	m_initialized = true;
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::Terminate()
{
	std::lock_guard lock(GetMutex());

	m_initialized = false;

	if (GetLogFile().is_open())
	{
		GetLogFile().flush();
		GetLogFile().close();
	}

	GetChannels().clear();
	GetMessages().clear();
	GetListeners().clear();
}

//////////////////////////////////////////////////////////////////////////
bool IsReservedChannelName(std::string_view name)
{
	return name == "all" || name == "All"
	    || name == "none" || name == "None"
	    || name == "*";
}

//////////////////////////////////////////////////////////////////////////
uint64_t CLogSystem::Register(std::string_view name, SColor const& color)
{
	uint64_t id = 0;
	bool const reserved = IsReservedChannelName(name);
	assert(!reserved && "Channel name is reserved (all, none, *)");

	if (!reserved)
	{
		std::lock_guard lock(GetMutex());

		id = HashChannelName(name);

		auto it = GetChannels().find(id);

		if (it == GetChannels().end())
		{
			GetChannels().emplace(id, SChannelData{std::string{name}, color});
		}
	}

	return id;
}

//////////////////////////////////////////////////////////////////////////
bool CLogSystem::SetLogLevel(std::string_view channelName, ELogLevel level)
{
	std::lock_guard lock(GetMutex());

	uint64_t const id = HashChannelName(channelName);
	auto const it = GetChannels().find(id);
	bool const found = it != GetChannels().end();

	if (found)
	{
		it->second.levelMask = level;
	}

	return found;
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::SetAllLogLevels(ELogLevel level)
{
	std::lock_guard lock(GetMutex());

	for (auto& [id, channel] : GetChannels())
	{
		channel.levelMask = level;
	}
}

//////////////////////////////////////////////////////////////////////////
ELogLevel CLogSystem::GetLogLevel(std::string_view channelName) const
{
	std::lock_guard lock(GetMutex());

	uint64_t const id = HashChannelName(channelName);
	auto it = GetChannels().find(id);

	if (it != GetChannels().end())
	{
		return it->second.levelMask;
	}

	return ELogLevel::All;
}

//////////////////////////////////////////////////////////////////////////
std::vector<std::string_view> CLogSystem::GetChannelNames() const
{
	std::lock_guard lock(GetMutex());

	std::vector<std::string_view> names;
	names.reserve(GetChannels().size());

	for (auto const& [id, channel] : GetChannels())
	{
		names.emplace_back(channel.name);
	}

	return names;
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::Write(uint64_t channelId, ELogLevel level, ETarget target, std::string_view message)
{
	if (!m_initialized)
	{
		// Fallback to stderr before logging system is initialized
		switch (level)
		{
			case ELogLevel::Info:
				std::cout << "[Pre-init] " << message << '\n' << std::flush;
				break;
			case ELogLevel::Warning:
				std::cout << "\033[33m[Pre-init] [WARNING] " << message << "\033[0m\n" << std::flush;
				break;
			case ELogLevel::Error:
				std::cerr << "\033[31m[Pre-init] [ERROR] " << message << "\033[0m\n" << std::flush;
				break;
			case ELogLevel::None:
			default:
				assert(false && "Invalid log level for message");
				break;
		}
	}
	else
	{
		std::lock_guard lock(GetMutex());

		auto it = GetChannels().find(channelId);
		bool const channelFound = it != GetChannels().end();

		if (channelFound)
		{
			SChannelData const& channel = it->second;

			// Check filtering (bitmask: message level must be in channel's allowed levels)
			if ((level & channel.levelMask) != ELogLevel::None)
			{
				auto const now = std::chrono::system_clock::now();
				auto const elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - GetStartTime());

				SLogMessage const& msg = GetMessages().emplace_back(
					now,
					static_cast<uint64_t>(elapsed.count()),
					level,
					target,
					channel.name,
					std::string{message},
					channel.color.r,
					channel.color.g,
					channel.color.b
				);

				if (GetMessages().size() > MaxMessages)
				{
					GetMessages().pop_front();
				}

				if ((target & ETarget::Terminal) != ETarget::None)
				{
					WriteToTerminal(msg);
				}

				if ((target & ETarget::File) != ETarget::None)
				{
					WriteToFile(msg);
				}

				if ((target & ETarget::Console) != ETarget::None)
				{
					NotifyListeners(msg);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
std::string_view CLogSystem::GetChannelNameById(uint64_t channelId) const
{
	std::lock_guard lock(GetMutex());

	auto it = GetChannels().find(channelId);

	if (it != GetChannels().end())
	{
		return it->second.name;
	}

	return "";
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::RegisterListener(void* key, LogMessageCallback callback)
{
	std::lock_guard lock(GetMutex());
	GetListeners().emplace_back(key, std::move(callback));
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::FlushTo(void* key)
{
	std::lock_guard lock(GetMutex());

	auto it = std::find_if(GetListeners().begin(), GetListeners().end(),
		[key](SListener const& listener) { return listener.key == key; });

	if (it != GetListeners().end())
	{
		// Replay all messages to this listener
		for (auto const& msg : GetMessages())
		{
			it->callback(msg);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::UnregisterListener(void* key)
{
	std::lock_guard lock(GetMutex());
	GetListeners().erase(
		std::remove_if(GetListeners().begin(), GetListeners().end(),
			[key](SListener const& listener) { return listener.key == key; }),
		GetListeners().end()
	);
}

//////////////////////////////////////////////////////////////////////////
CLog::CLog(std::string_view name, SColor const& color)
{
	m_id = GetLogSystem().Register(name, color);
}

//////////////////////////////////////////////////////////////////////////
void CLog::Write(ELogLevel level, ETarget target, std::string_view message) const
{
	GetLogSystem().Write(m_id, level, target, message);
}

//////////////////////////////////////////////////////////////////////////
std::string_view CLog::GetName() const
{
	return GetLogSystem().GetChannelNameById(m_id);
}
#endif // TGE_LOGGING_ENABLED

//////////////////////////////////////////////////////////////////////////
CLogSystem& GetLogSystem()
{
	static CLogSystem logSystem;
	return logSystem;
}
#ifndef TGE_LOGGING_ENABLED
void CLogSystem::Initialize(std::string_view, std::string_view, std::string_view) {}
void CLogSystem::Terminate() {}
bool CLogSystem::IsInitialized() const { return false; }
uint64_t CLogSystem::Register(std::string_view, SColor const&) { return 0; }
bool CLogSystem::SetLogLevel(std::string_view, ELogLevel) { return false; }
void CLogSystem::SetAllLogLevels(ELogLevel) {}
ELogLevel CLogSystem::GetLogLevel(std::string_view) const { return ELogLevel::All; }
std::vector<std::string_view> CLogSystem::GetChannelNames() const { return {}; }
void CLogSystem::Write(uint64_t, ELogLevel, ETarget, std::string_view) {}
std::string_view CLogSystem::GetChannelNameById(uint64_t) const { return ""; }
void CLogSystem::RegisterListener(void*, LogMessageCallback) {}
void CLogSystem::FlushTo(void*) {}
void CLogSystem::UnregisterListener(void*) {}

CLog::CLog(std::string_view, SColor const&) {}
std::string_view CLog::GetName() const { return ""; }
void CLog::Write(ELogLevel, ETarget, std::string_view) const {}
#endif // !TGE_LOGGING_ENABLED
} // namespace Tge::Logging
