#include <tge/io/directory_watch.hpp>
#include "native_directory_watch.hpp"
#include <tge/assert.hpp>
#include <utility>

#if defined(TGE_PLATFORM_WINDOWS)
#include <cstddef>
#include <string_view>
#elif defined(TGE_PLATFORM_LINUX)
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <deque>
#include <string_view>
#include <sys/inotify.h>
#include <unistd.h>
#include <vector>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::IO
{
namespace
{
#if defined(TGE_PLATFORM_WINDOWS)
constexpr DWORD BufferSize{ 64 * 1024 };
constexpr DWORD NotifyFilter{ FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE };

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
// Windows cancels a thread's pending reads when that thread exits, so the threads that read must outlive the watch.
std::error_code IssueRead(SNativeDirectoryWatch& watch)
{
	BOOL const isIssued{ ReadDirectoryChangesW(watch.folder, watch.buffer.data(), BufferSize, (watch.scope == EWatchScope::Tree) ? TRUE : FALSE,
		NotifyFilter, nullptr, &watch.overlapped, nullptr) };

	watch.isReadPending = isIssued != 0;

	return watch.isReadPending ? std::error_code{} : GetLastSystemError();
}

//////////////////////////////////////////////////////////////////////////
// A folder's own last write says only that something in it changed, which comes as an entry of its own; inotify sends
// no such entry, so it is left out to report the same changes on both.
void CollectPaths(SNativeDirectoryWatch const& watch, SDirectoryChanges& changes)
{
	std::byte const* pEntry{ reinterpret_cast<std::byte const*>(watch.buffer.data()) };
	bool isMore{ true };
	std::error_code error{};

	while (isMore)
	{
		FILE_NOTIFY_INFORMATION const* const pInfo{ reinterpret_cast<FILE_NOTIFY_INFORMATION const*>(pEntry) };
		std::filesystem::path path{ std::wstring_view{ pInfo->FileName, pInfo->FileNameLength / sizeof(WCHAR) } };

		if (pInfo->Action != FILE_ACTION_MODIFIED || !std::filesystem::is_directory(watch.root / path, error))
		{
			changes.paths.emplace_back(std::move(path));
		}

		isMore = pInfo->NextEntryOffset != 0;
		pEntry += pInfo->NextEntryOffset;
	}
}

//////////////////////////////////////////////////////////////////////////
// The system writes into the buffer until a cancelled read completes, so it is waited for before anything goes.
void Release(SNativeDirectoryWatch& watch)
{
	if (watch.isReadPending)
	{
		DWORD numBytes{ 0 };

		CancelIoEx(watch.folder, &watch.overlapped);
		GetOverlappedResult(watch.folder, &watch.overlapped, &numBytes, TRUE);
		watch.isReadPending = false;
	}

	if (watch.folder != INVALID_HANDLE_VALUE)
	{
		CloseHandle(watch.folder);
		watch.folder = INVALID_HANDLE_VALUE;
	}

	if (watch.overlapped.hEvent != nullptr)
	{
		CloseHandle(watch.overlapped.hEvent);
		watch.overlapped.hEvent = nullptr;
	}
}
#elif defined(TGE_PLATFORM_LINUX)
constexpr uint32_t WatchedEvents{ IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF | IN_ONLYDIR };
constexpr uint32_t RootGoneEvents{ IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED | IN_UNMOUNT };
constexpr size_t ReadBufferSize{ 16 * 1024 };

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ errno, std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
bool IsWithin(std::filesystem::path const& path, std::filesystem::path const& base)
{
	auto const [baseEnd, pathEnd]{ std::mismatch(base.begin(), base.end(), path.begin(), path.end()) };

	return baseEnd == base.end();
}

//////////////////////////////////////////////////////////////////////////
// True for a folder newly watched. Symlinked folders are followed, as inotify follows them; one already watched comes
// back with its own descriptor, so a symlink loop ends there. A folder that went away or cannot be read is skipped,
// but running out of watches fails: a tree watched in part would miss changes without a word.
std::expected<bool, std::error_code> AddWatch(SNativeDirectoryWatch& watch, std::filesystem::path const& relative)
{
	std::filesystem::path const full{ watch.root / relative };
	int const descriptor{ ::inotify_add_watch(watch.descriptor, full.c_str(), WatchedEvents) };
	std::expected<bool, std::error_code> result{ false };

	if (descriptor >= 0)
	{
		result = watch.folders.try_emplace(descriptor, relative).second;
	}
	else if (errno == ENOSPC || errno == ENOMEM)
	{
		result = std::unexpected{ GetLastSystemError() };
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
// Breadth-first, each folder watched before it is listed, so a folder made meanwhile raises an event of its own.
std::error_code WatchBelow(SNativeDirectoryWatch& watch, std::filesystem::path const& start)
{
	std::deque<std::filesystem::path> pending{ start };
	std::error_code result{};

	while (!pending.empty() && result == std::error_code{})
	{
		std::filesystem::path const relative{ std::move(pending.front()) };
		std::error_code error{};

		pending.pop_front();

		for (std::filesystem::directory_iterator entry{ watch.root / relative, error }, end{};
			error == std::error_code{} && result == std::error_code{} && entry != end; entry.increment(error))
		{
			std::error_code typeError{};

			if (entry->is_directory(typeError))
			{
				std::filesystem::path child{ relative / entry->path().filename() };
				std::expected<bool, std::error_code> const added{ AddWatch(watch, child) };

				if (!added.has_value())
				{
					result = added.error();
				}
				else if (*added)
				{
					pending.push_back(std::move(child));
				}
			}
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
void ForgetBelow(SNativeDirectoryWatch& watch, std::filesystem::path const& relative)
{
	auto folder{ watch.folders.begin() };

	while (folder != watch.folders.end())
	{
		if (folder->first != watch.rootWatch && IsWithin(folder->second, relative))
		{
			::inotify_rm_watch(watch.descriptor, folder->first);
			folder = watch.folders.erase(folder);
		}
		else
		{
			++folder;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// A folder moved out loses its watches, else it would go on reporting under its old name.
void ReadEvent(SNativeDirectoryWatch& watch, inotify_event const& event, SDirectoryChanges& changes, std::vector<std::filesystem::path>& newFolders,
	bool& isRootGone, bool& shouldWalkAgain)
{
	auto const folder{ watch.folders.find(event.wd) };
	bool const isTreeFolder{ watch.scope == EWatchScope::Tree && (event.mask & IN_ISDIR) != 0 };

	if ((event.mask & IN_Q_OVERFLOW) != 0)
	{
		changes.hasOverflowed = true;
		shouldWalkAgain = true;
	}
	else if (event.wd == watch.rootWatch && (event.mask & RootGoneEvents) != 0)
	{
		isRootGone = true;
	}
	else if (folder != watch.folders.end() && (event.mask & IN_IGNORED) != 0)
	{
		watch.folders.erase(folder);
	}
	else if (folder != watch.folders.end())
	{
		std::string_view const name{ (event.len > 0) ? std::string_view{ event.name } : std::string_view{} };
		std::filesystem::path path{ name.empty() ? folder->second : folder->second / name };
		bool const isArrival{ (event.mask & (IN_CREATE | IN_MOVED_TO)) != 0 };
		std::error_code linkError{};
		// A symlink comes without IN_ISDIR even when it leads to a folder; AddWatch skips one that leads elsewhere.
		bool const isNewLink{ watch.scope == EWatchScope::Tree && isArrival && (event.mask & IN_ISDIR) == 0
			&& std::filesystem::is_symlink(std::filesystem::symlink_status(watch.root / path, linkError)) };

		if ((isTreeFolder || isNewLink) && isArrival)
		{
			newFolders.push_back(path);
		}
		else if (isTreeFolder && (event.mask & IN_MOVED_FROM) != 0)
		{
			ForgetBelow(watch, path);
		}

		if (!path.empty())
		{
			changes.paths.push_back(std::move(path));
		}
	}
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
CDirectoryWatch::CDirectoryWatch() = default;

//////////////////////////////////////////////////////////////////////////
CDirectoryWatch::~CDirectoryWatch() = default;

//////////////////////////////////////////////////////////////////////////
// Changes made before the owner first reads wait in the system, so it may watch the waitable later.
std::expected<void, std::error_code> CDirectoryWatch::Initialize(std::filesystem::path const& folder, EWatchScope scope)
{
	TGE_ASSERT(m_pWatch == nullptr, "Directory watch initialized twice");

	auto pWatch{ std::make_unique<SNativeDirectoryWatch>() };
	std::error_code error{};

	pWatch->scope = scope;
	pWatch->root = folder;

#if defined(TGE_PLATFORM_WINDOWS)
	BY_HANDLE_FILE_INFORMATION information{};

	pWatch->folder = CreateFileW(folder.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

	if (pWatch->folder == INVALID_HANDLE_VALUE || GetFileInformationByHandle(pWatch->folder, &information) == 0)
	{
		error = GetLastSystemError();
	}
	else if ((information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		error = std::make_error_code(std::errc::not_a_directory);
	}
	else if (pWatch->overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr); pWatch->overlapped.hEvent == nullptr)
	{
		error = GetLastSystemError();
	}
	else
	{
		pWatch->buffer.resize(BufferSize / sizeof(DWORD));
		error = IssueRead(*pWatch);
	}

	if (error != std::error_code{})
	{
		Release(*pWatch);
	}
#elif defined(TGE_PLATFORM_LINUX)
	pWatch->descriptor = ::inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	pWatch->rootWatch = (pWatch->descriptor >= 0) ? ::inotify_add_watch(pWatch->descriptor, folder.c_str(), WatchedEvents) : -1;

	if (pWatch->rootWatch < 0)
	{
		error = GetLastSystemError();
	}
	else
	{
		pWatch->folders.emplace(pWatch->rootWatch, std::filesystem::path{});
		error = (scope == EWatchScope::Tree) ? WatchBelow(*pWatch, std::filesystem::path{}) : std::error_code{};
	}

	if (error != std::error_code{} && pWatch->descriptor >= 0)
	{
		::close(pWatch->descriptor);
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	if (error == std::error_code{})
	{
		m_pWatch = std::move(pWatch);
	}

	return (error == std::error_code{}) ? std::expected<void, std::error_code>{} : std::unexpected{ error };
}

//////////////////////////////////////////////////////////////////////////
void CDirectoryWatch::Terminate()
{
	if (m_pWatch != nullptr)
	{
#if defined(TGE_PLATFORM_WINDOWS)
		Release(*m_pWatch);
#elif defined(TGE_PLATFORM_LINUX)
		::close(m_pWatch->descriptor);
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

		m_pWatch.reset();
	}
}

//////////////////////////////////////////////////////////////////////////
// Never waits. An error means the watch ended, the folder gone or unreadable: anything in it may have changed, and
// the owner unwatches the waitable before anything else, as it may stay signalled.
std::expected<SDirectoryChanges, std::error_code> CDirectoryWatch::ReadChanges()
{
	TGE_ASSERT(m_pWatch != nullptr, "Changes read from a directory watch that is not running");

	SNativeDirectoryWatch& watch{ *m_pWatch };
	std::expected<SDirectoryChanges, std::error_code> result{ SDirectoryChanges{} };

#if defined(TGE_PLATFORM_WINDOWS)
	DWORD numBytes{ 0 };

	if (!watch.isReadPending)
	{
		result = std::unexpected{ std::make_error_code(std::errc::operation_canceled) };
	}
	else if (GetOverlappedResult(watch.folder, &watch.overlapped, &numBytes, FALSE) != 0)
	{
		watch.isReadPending = false;
		result->hasOverflowed = numBytes == 0;

		if (numBytes != 0)
		{
			CollectPaths(watch, *result);
		}

		if (std::error_code const reissued{ IssueRead(watch) }; reissued != std::error_code{})
		{
			result = std::unexpected{ reissued };
		}
	}
	else if (DWORD const failure{ GetLastError() }; failure == ERROR_NOTIFY_ENUM_DIR)
	{
		watch.isReadPending = false;
		result->hasOverflowed = true;

		if (std::error_code const reissued{ IssueRead(watch) }; reissued != std::error_code{})
		{
			result = std::unexpected{ reissued };
		}
	}
	else if (failure != ERROR_IO_INCOMPLETE)
	{
		watch.isReadPending = false;
		result = std::unexpected{ std::error_code{ static_cast<int>(failure), std::system_category() } };
	}
#elif defined(TGE_PLATFORM_LINUX)
	alignas(inotify_event) std::array<char, ReadBufferSize> buffer{};
	std::vector<std::filesystem::path> newFolders{};
	bool isRootGone{ false };
	bool shouldWalkAgain{ false };
	ssize_t count{ ::read(watch.descriptor, buffer.data(), buffer.size()) };

	while (count > 0)
	{
		size_t offset{ 0 };

		while (offset < static_cast<size_t>(count))
		{
			inotify_event const* const pEvent{ reinterpret_cast<inotify_event const*>(buffer.data() + offset) };

			ReadEvent(watch, *pEvent, *result, newFolders, isRootGone, shouldWalkAgain);
			offset += sizeof(inotify_event) + pEvent->len;
		}

		count = ::read(watch.descriptor, buffer.data(), buffer.size());
	}

	std::error_code error{ (count < 0 && errno != EAGAIN && errno != EINTR) ? GetLastSystemError() : std::error_code{} };

	if (isRootGone)
	{
		error = std::make_error_code(std::errc::no_such_file_or_directory);
	}

	for (std::filesystem::path const& folder : newFolders)
	{
		if (error == std::error_code{})
		{
			std::expected<bool, std::error_code> const added{ AddWatch(watch, folder) };

			if (!added.has_value())
			{
				error = added.error();
			}
			else if (*added)
			{
				error = WatchBelow(watch, folder);
			}
		}
	}

	// Changes were lost, maybe the root's own end among them and folders made or moved, so the root is asked for afresh
	// and the tree walked again.
	if (error == std::error_code{} && shouldWalkAgain)
	{
		if (::inotify_add_watch(watch.descriptor, watch.root.c_str(), WatchedEvents) != watch.rootWatch)
		{
			error = std::make_error_code(std::errc::no_such_file_or_directory);
		}
		else if (watch.scope == EWatchScope::Tree)
		{
			ForgetBelow(watch, std::filesystem::path{});
			error = WatchBelow(watch, std::filesystem::path{});
		}
	}

	if (error != std::error_code{})
	{
		result = std::unexpected{ error };
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return result;
}

//////////////////////////////////////////////////////////////////////////
Threading::SWaitable CDirectoryWatch::GetWaitable() const
{
	TGE_ASSERT(m_pWatch != nullptr, "Waitable of a directory watch that is not running");

#if defined(TGE_PLATFORM_WINDOWS)
	return Threading::SWaitable{ reinterpret_cast<uintptr_t>(m_pWatch->overlapped.hEvent), Threading::EWaitableKind::Event };
#elif defined(TGE_PLATFORM_LINUX)
	return Threading::SWaitable{ static_cast<uintptr_t>(m_pWatch->descriptor), Threading::EWaitableKind::Event };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}
} // namespace Tge::IO
