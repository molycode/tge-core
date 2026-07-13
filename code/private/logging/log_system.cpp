#include <tge/logging/log_system.hpp>
#include <tge/logging/log.hpp>
#include <tge/color.hpp>
#include <tge/assert.hpp>
#include <tge/platform.hpp>

#ifdef TGE_LOGGING_ENABLED
#include <algorithm>
#include <cassert>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <vector>
#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#endif // defined(TGE_PLATFORM_WINDOWS)
#endif // TGE_LOGGING_ENABLED

namespace Tge::Logging
{
#ifdef TGE_LOGGING_ENABLED
namespace
{
constexpr size_t MaxMessages = 1024;

// Construct-on-first-use idiom to avoid static initialization order fiasco.
// CLog instances are global statics that register during static init,
// so this container must be available before main() runs.

//////////////////////////////////////////////////////////////////////////
std::unordered_map<uint64_t, CLog*>& GetLoggers()
{
	static std::unordered_map<uint64_t, CLog*> loggers;
	return loggers;
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
std::vector<SListener>& GetListeners()
{
	static std::vector<SListener> listeners;
	return listeners;
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
			for (auto& [id, pLog] : GetLoggers())
			{
				pLog->SetLevelMask(defaultLevel);
			}
		}

		// Apply channel-specific overrides
		for (auto const& [name, level] : channelOverrides)
		{
			uint64_t const id = HashChannelName(name);
			auto it = GetLoggers().find(id);

			if (it != GetLoggers().end())
			{
				it->second->SetLevelMask(level);
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
void FormatWallClockTimestamp(std::chrono::system_clock::time_point timestamp, char* buffer, size_t bufferSize)
{
	auto const timeT = std::chrono::system_clock::to_time_t(timestamp);
	auto const ms    = std::chrono::duration_cast<std::chrono::milliseconds>(
	                       timestamp.time_since_epoch()) % 1000;
	std::tm tm{};
#if defined(TGE_PLATFORM_WINDOWS)
	localtime_s(&tm, &timeT);
#elif defined(TGE_PLATFORM_LINUX)
	localtime_r(&timeT, &tm);
#else
#	error "localtime_r/localtime_s: unhandled platform — add an implementation here"
#endif // defined(TGE_PLATFORM_WINDOWS)
	char timePart[16];
	std::strftime(timePart, sizeof(timePart), "%H:%M:%S", &tm);
	std::snprintf(buffer, bufferSize, "%s.%03lld", timePart, static_cast<long long>(ms.count()));
}

//////////////////////////////////////////////////////////////////////////
std::string FormatMessageForTerminal(SLogMessage const& msg)
{
	if (msg.level == ELogLevel::Info)
	{
		return std::format("[{}] [{}] {}", msg.formattedTimestamp, msg.channelName, msg.message);
	}

	return std::format("[{}] [{}] [{}] {}", msg.formattedTimestamp, LevelToString(msg.level), msg.channelName, msg.message);
}

//////////////////////////////////////////////////////////////////////////
std::string FormatMessage(SLogMessage const& msg)
{
	return std::format("[{}] [{}] [{}] {}", msg.formattedTimestamp, LevelToString(msg.level), msg.channelName, msg.message);
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
		GetLogFile() << FormatMessage(msg) << '\n';
		GetLogFile().flush();
	}
}

} // namespace

//////////////////////////////////////////////////////////////////////////
void CLogSystem::Initialize(std::string_view prefix, std::string_view logsDir, std::string_view configDir, ETimestampMode timestampMode)
{
	std::lock_guard lock(GetMutex());

#if defined(TGE_PLATFORM_WINDOWS)
	// Switch console output to UTF-8 and enable ANSI escape code processing
	SetConsoleOutputCP(CP_UTF8);

	for (DWORD handle : { STD_OUTPUT_HANDLE, STD_ERROR_HANDLE })
	{
		HANDLE h = GetStdHandle(handle);
		DWORD mode{ 0 };

		if (GetConsoleMode(h, &mode))
		{
			SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		}
	}
#endif // defined(TGE_PLATFORM_WINDOWS)

	m_timestampMode = timestampMode;

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
#if defined(TGE_PLATFORM_WINDOWS)
		localtime_s(&tm, &time_t);
#elif defined(TGE_PLATFORM_LINUX)
		localtime_r(&time_t, &tm);
#else
#	error "localtime_r/localtime_s: unhandled platform — add an implementation here"
#endif // defined(TGE_PLATFORM_WINDOWS)

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

	GetLoggers().clear();
}

//////////////////////////////////////////////////////////////////////////
bool IsReservedChannelName(std::string_view name)
{
	return name == "all" || name == "All"
	    || name == "none" || name == "None"
	    || name == "*";
}

//////////////////////////////////////////////////////////////////////////
uint64_t CLogSystem::Register(std::string_view name)
{
	uint64_t id = 0;
	bool const reserved = IsReservedChannelName(name);
	assert(!reserved && "Channel name is reserved (all, none, *)");

	if (!reserved)
	{
		std::lock_guard lock(GetMutex());
		id = HashChannelName(name);
		assert(GetLoggers().find(id) == GetLoggers().end() && "Duplicate channel name registered");
	}

	return id;
}

//////////////////////////////////////////////////////////////////////////
bool CLogSystem::SetLogLevel(std::string_view channelName, ELogLevel level)
{
	std::lock_guard lock(GetMutex());

	uint64_t const id = HashChannelName(channelName);
	auto const it = GetLoggers().find(id);
	bool const found = it != GetLoggers().end();

	if (found)
	{
		it->second->SetLevelMask(level);
	}

	return found;
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::SetAllLogLevels(ELogLevel level)
{
	std::lock_guard lock(GetMutex());

	for (auto& [id, pLog] : GetLoggers())
	{
		pLog->SetLevelMask(level);
	}
}

//////////////////////////////////////////////////////////////////////////
ELogLevel CLogSystem::GetLogLevel(std::string_view channelName) const
{
	std::lock_guard lock(GetMutex());

	uint64_t const id = HashChannelName(channelName);
	auto it = GetLoggers().find(id);

	if (it != GetLoggers().end())
	{
		return it->second->GetLevelMask();
	}

	return ELogLevel::All;
}

//////////////////////////////////////////////////////////////////////////
std::vector<std::string_view> CLogSystem::GetChannelNames() const
{
	std::lock_guard lock(GetMutex());

	std::vector<std::string_view> names;
	names.reserve(GetLoggers().size());

	for (auto const& [id, pLog] : GetLoggers())
	{
		names.emplace_back(pLog->GetName());
	}

	return names;
}

//////////////////////////////////////////////////////////////////////////
std::string_view CLogSystem::GetChannelNameById(uint64_t channelId) const
{
	std::lock_guard lock(GetMutex());

	auto it = GetLoggers().find(channelId);

	if (it != GetLoggers().end())
	{
		return it->second->GetName();
	}

	return "";
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::RegisterListener(void* key, LogMessageCallback callback, EMessageFormat format, EHistory history)
{
	std::vector<std::shared_ptr<SLogMessage>> historyToFlush;
	SListener listenerSnapshot;

	{
		std::lock_guard lock(GetMutex());
		TGE_ASSERT(std::none_of(GetListeners().begin(), GetListeners().end(),
			[key](SListener const& l) { return l.key == key; }),
			"Listener with this key is already registered");

		if (history == EHistory::Flush)
		{
			for (auto& [id, pLog] : GetLoggers())
			{
				pLog->AppendHistoryTo(historyToFlush);
			}

			std::sort(historyToFlush.begin(), historyToFlush.end(),
				[](auto const& a, auto const& b) { return a->elapsedMs < b->elapsedMs; });

			listenerSnapshot = { key, callback, format };
		}

		GetListeners().emplace_back(key, std::move(callback), format);
	}

	for (auto const& msg : historyToFlush)
	{
		if (listenerSnapshot.format == EMessageFormat::Formatted)
		{
			SLogMessage formatted{ *msg };
			formatted.message = FormatMessage(*msg);
			listenerSnapshot.callback(formatted);
		}
		else
		{
			listenerSnapshot.callback(*msg);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::UnregisterListener(void* key)
{
	std::lock_guard lock(GetMutex());
	GetListeners().erase(
		std::remove_if(GetListeners().begin(), GetListeners().end(),
			[key](SListener const& l) { return l.key == key; }),
		GetListeners().end()
	);
}

//////////////////////////////////////////////////////////////////////////
void CLogSystem::DispatchListeners()
{
	std::vector<CLog*> loggers;

	{
		std::lock_guard lock(GetMutex());
		loggers.reserve(GetLoggers().size());

		for (auto& [id, pLog] : GetLoggers())
		{
			loggers.push_back(pLog);
		}
	}

	for (auto* pLog : loggers)
	{
		pLog->DispatchPending();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CLogSystem::IsInitialized() const
{
	return m_initialized;
}

//////////////////////////////////////////////////////////////////////////
ETimestampMode CLogSystem::GetTimestampMode() const
{
	return m_timestampMode;
}

//////////////////////////////////////////////////////////////////////////
CLog::CLog(std::string_view name, SColor const& color)
	: m_name{ name }
	, m_color{ color }
{
	m_id = GetLogSystem().Register(name);

	std::lock_guard lock(GetMutex());
	GetLoggers().emplace(m_id, this);
}

//////////////////////////////////////////////////////////////////////////
CLog::~CLog()
{
	std::lock_guard lock(GetMutex());
	GetLoggers().erase(m_id);
}

//////////////////////////////////////////////////////////////////////////
void CLog::Write(ELogLevel level, ETarget target, std::string message) const
{
	if (!GetLogSystem().IsInitialized())
	{
		// The mask is set on the channel itself, so it is meaningful before the system comes up — honour it
		// here too, or SetLogLevel silently does nothing to anything logged this early.
		if ((level & m_levelMask) != ELogLevel::None)
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
	}
	else
	{
		std::lock_guard lock(GetMutex());

		// Check filtering (bitmask: message level must be in channel's allowed levels)
		if ((level & m_levelMask) != ELogLevel::None)
		{
			auto const now = std::chrono::system_clock::now();
			auto const elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - GetStartTime());

			char tsBuf[32];

			if (GetLogSystem().GetTimestampMode() == ETimestampMode::WallClock)
			{
				FormatWallClockTimestamp(now, tsBuf, sizeof(tsBuf));
			}
			else
			{
				FormatTimestamp(static_cast<uint64_t>(elapsed.count()), tsBuf, sizeof(tsBuf));
			}

			if ((target & ETarget::Listeners) != ETarget::None)
			{
				// Console path: heap-allocate a shared SLogMessage so both the history
				// deque and the pending-dispatch queue own it without copying strings.
				auto msg = std::make_shared<SLogMessage>(SLogMessage{
					now,
					static_cast<uint64_t>(elapsed.count()),
					level,
					target,
					std::string_view{m_name},
					std::move(message),
					std::string{tsBuf},
					m_color.r,
					m_color.g,
					m_color.b
				});

				if ((target & ETarget::Terminal) != ETarget::None)
				{
					WriteToTerminal(*msg);
				}

				if ((target & ETarget::File) != ETarget::None)
				{
					WriteToFile(*msg);
				}

				m_pendingDispatch.push_back(msg);
				m_history.push_back(std::move(msg));

				if (m_history.size() > MaxMessages)
				{
					m_history.pop_front();
				}
			}
			else
			{
				// Terminal/File only: build a local SLogMessage (stack-allocated struct,
				// no heap for channelName or timestamp — SSO covers both).
				SLogMessage msg{
					now,
					static_cast<uint64_t>(elapsed.count()),
					level,
					target,
					std::string_view{m_name},
					std::move(message),
					std::string{tsBuf},
					m_color.r,
					m_color.g,
					m_color.b
				};

				if ((target & ETarget::Terminal) != ETarget::None)
				{
					WriteToTerminal(msg);
				}

				if ((target & ETarget::File) != ETarget::None)
				{
					WriteToFile(msg);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CLog::RegisterListener(void* key, LogMessageCallback callback, EMessageFormat format)
{
	std::lock_guard lock(GetMutex());
	TGE_ASSERT(std::none_of(m_listeners.begin(), m_listeners.end(),
		[key](SListener const& l) { return l.key == key; }),
		"Listener with this key is already registered — did you forget to UnregisterListener?");

	m_listeners.emplace_back(key, std::move(callback), format);
}

//////////////////////////////////////////////////////////////////////////
void CLog::UnregisterListener(void* key)
{
	std::lock_guard lock(GetMutex());
	m_listeners.erase(
		std::remove_if(m_listeners.begin(), m_listeners.end(),
			[key](SListener const& listener) { return listener.key == key; }),
		m_listeners.end()
	);
}

//////////////////////////////////////////////////////////////////////////
void CLog::FlushTo(void* key)
{
	std::lock_guard lock(GetMutex());
	auto listenerIt = std::find_if(m_listeners.begin(), m_listeners.end(),
		[key](SListener const& listener) { return listener.key == key; });

	if (listenerIt != m_listeners.end())
	{
		for (auto const& msg : m_history)
		{
			if (listenerIt->format == EMessageFormat::Formatted)
			{
				SLogMessage formatted{ *msg };
				formatted.message = FormatMessage(*msg);
				listenerIt->callback(formatted);
			}
			else
			{
				listenerIt->callback(*msg);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CLog::DispatchPending()
{
	std::vector<std::shared_ptr<SLogMessage>> toDispatch;
	std::vector<SListener> listenersSnapshot;
	std::vector<SListener> globalSnapshot;

	{
		std::lock_guard lock(GetMutex());
		std::swap(toDispatch, m_pendingDispatch);
		listenersSnapshot = m_listeners;
		globalSnapshot    = GetListeners();
	}

	for (auto const& msg : toDispatch)
	{
		std::string formattedText;
		bool formattedComputed{ false };

		auto dispatch = [&](SListener const& listener)
		{
			if (listener.format == EMessageFormat::Formatted)
			{
				if (!formattedComputed)
				{
					formattedText    = FormatMessage(*msg);
					formattedComputed = true;
				}

				SLogMessage formatted{ *msg };
				formatted.message = formattedText;
				listener.callback(formatted);
			}
			else
			{
				listener.callback(*msg);
			}
		};

		for (auto const& listener : listenersSnapshot)
		{
			dispatch(listener);
		}

		for (auto const& listener : globalSnapshot)
		{
			dispatch(listener);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CLog::AppendHistoryTo(std::vector<std::shared_ptr<SLogMessage>>& out) const
{
	out.insert(out.end(), m_history.begin(), m_history.end());
}

//////////////////////////////////////////////////////////////////////////
void CLog::SetLevelMask(ELogLevel level)
{
	m_levelMask = level;
}

//////////////////////////////////////////////////////////////////////////
ELogLevel CLog::GetLevelMask() const
{
	return m_levelMask;
}

#endif // TGE_LOGGING_ENABLED

//////////////////////////////////////////////////////////////////////////
CLogSystem& GetLogSystem()
{
	static CLogSystem logSystem;
	return logSystem;
}
#ifndef TGE_LOGGING_ENABLED
void CLogSystem::Initialize(std::string_view, std::string_view, std::string_view, ETimestampMode) {}
void CLogSystem::Terminate() {}
bool CLogSystem::IsInitialized() const { return false; }
ETimestampMode CLogSystem::GetTimestampMode() const { return ETimestampMode::Elapsed; }
uint64_t CLogSystem::Register(std::string_view) { return 0; }
bool CLogSystem::SetLogLevel(std::string_view, ELogLevel) { return false; }
void CLogSystem::SetAllLogLevels(ELogLevel) {}
ELogLevel CLogSystem::GetLogLevel(std::string_view) const { return ELogLevel::All; }
std::vector<std::string_view> CLogSystem::GetChannelNames() const { return {}; }
std::string_view CLogSystem::GetChannelNameById(uint64_t) const { return ""; }
void CLogSystem::RegisterListener(void*, LogMessageCallback, EMessageFormat, EHistory) {}
void CLogSystem::UnregisterListener(void*) {}
void CLogSystem::DispatchListeners() {}

CLog::CLog(std::string_view name, SColor const& color) : m_name{ name }, m_color{ color } {}
CLog::~CLog() {}
void CLog::Write(ELogLevel, ETarget, std::string) const {}
void CLog::RegisterListener(void*, LogMessageCallback, EMessageFormat) {}
void CLog::UnregisterListener(void*) {}
void CLog::FlushTo(void*) {}
void CLog::AppendHistoryTo(std::vector<std::shared_ptr<SLogMessage>>&) const {}
void CLog::DispatchPending() {}
void CLog::SetLevelMask(ELogLevel) {}
ELogLevel CLog::GetLevelMask() const { return ELogLevel::All; }
#endif // !TGE_LOGGING_ENABLED
} // namespace Tge::Logging
