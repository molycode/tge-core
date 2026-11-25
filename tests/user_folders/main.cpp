#include <tge/init/init.hpp>
#include <tge/io/get_user_folder.hpp>
#include <tge/io/read_whole_file.hpp>
#include <tge/io/write_file_atomically.hpp>
#include <tge/logging/log_system.hpp>
#include <tge/module/app.hpp>
#include <tge/module/frame_loop.hpp>
#include <tge/threading/job_group.hpp>
#include <emscripten/em_asm.h>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
constexpr uint32_t NumFrames{ 10u };
constexpr uint32_t NumPagesPerJob{ 200u };
constexpr size_t   PageSize{ 1u << 16u };

//////////////////////////////////////////////////////////////////////////
std::filesystem::path GetFolder()
{
	return Tge::IO::GetUserFolder(Tge::IO::EUserFolder::Config).value_or(std::filesystem::path{}) / "app";
}

//////////////////////////////////////////////////////////////////////////
bool Write(std::string_view name, std::string_view text)
{
	return Tge::IO::WriteFileAtomically(GetFolder() / name, text).has_value();
}

//////////////////////////////////////////////////////////////////////////
std::string Read(std::string_view name)
{
	return Tge::IO::ReadWholeFile(GetFolder() / name, PageSize).value_or(std::string{ "unreadable" });
}

//////////////////////////////////////////////////////////////////////////
// Any of the pages written, but a whole one.
bool IsWholePage(std::string_view text)
{
	return text.size() == PageSize && text.find_first_not_of(text.front()) == std::string_view::npos;
}

//////////////////////////////////////////////////////////////////////////
// Writes `text` in its first frame, pages from a job in every one, and "last" while it shuts down.
class CWritingApp final : public Tge::IApp
{
public:

	explicit CWritingApp(std::string_view mode, std::string_view text)
		: m_mode{ mode }
		, m_text{ text }
	{}

	// Tge::IApp
	bool Initialize() override
	{
		std::error_code error{};

		Tge::Logging::GetLogSystem().Initialize("test", "", "");
		Tge::Logging::GetLogSystem().RegisterListener(this, [this](Tge::Logging::SLogMessage const& message)
		{
			m_numWarnings += (message.level == Tge::Logging::ELogLevel::Warning) ? 1u : 0u;
		});

		std::filesystem::create_directories(GetFolder(), error);

		return Tge::Initialize();
	}

	void Update(float deltaTime) override
	{
		// Not waited for: with workers the page is replaced while the tick stores.
		m_pages.SubmitJob([page = std::string(PageSize, static_cast<char>('a' + m_numFrames))]()
		{
			for (uint32_t index{ 0u }; index < NumPagesPerJob; ++index)
			{
				Write("page", page);
			}
		});

		if (m_numFrames == 0u)
		{
			m_isRight = Write("first", m_text);
		}

		Tge::Update();
		++m_numFrames;

		if (m_mode == "--crash" && m_numFrames == NumFrames)
		{
			EM_ASM({ process.exit(0); });
		}
	}

	bool CanRun() const override
	{
		return m_numFrames < NumFrames;
	}

	int Terminate() override
	{
		m_pages.Wait();

		m_isRight = Write("last", m_text) && m_isRight && Read("first") == m_text && IsWholePage(Read("page"));
		m_isRight = m_isRight && m_numWarnings == ((m_mode == "--refused") ? 1u : 0u);

		Tge::Logging::GetLogSystem().UnregisterListener(this);
		Tge::Terminate();
		Tge::Logging::GetLogSystem().Terminate();

		return m_isRight ? 0 : 1;
	}
	// ~Tge::IApp

private:

	Tge::Threading::CJobGroup m_pages{};
	std::string               m_mode{};
	std::string               m_text{};
	uint32_t                  m_numFrames{ 0u };
	uint32_t                  m_numWarnings{ 0u };
	bool                      m_isRight{ false };
};
} // namespace

//////////////////////////////////////////////////////////////////////////
// --read <first> <last> checks what earlier runs left before anything else runs; the others run the app.
int main(int argc, char** argv)
{
	std::string_view const mode{ argv[1] };
	std::string_view const text{ argv[2] };
	int status{ 1 };

	if (mode == "--read")
	{
		std::string const first{ Read("first") };
		std::string const last{ Read("last") };

		std::printf("first '%s', last '%s'\n", first.c_str(), last.c_str());
		status = (first == text && last == argv[3] && IsWholePage(Read("page"))) ? 0 : 1;
	}
	else
	{
		status = Tge::RunFrameLoop(std::make_unique<CWritingApp>(mode, text));
	}

	return status;
}
