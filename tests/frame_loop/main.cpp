#include <tge/module/app.hpp>
#include <tge/module/frame_loop.hpp>
#include <emscripten/eventloop.h>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>

namespace
{
constexpr uint32_t NumFrames{ 3u };
constexpr int      RefusedStatus{ 3 };

bool     gHasMainReturned{ false };
uint32_t gNumTimers{ 0u };

//////////////////////////////////////////////////////////////////////////
void OnTimer(void*)
{
	++gNumTimers;
}

//////////////////////////////////////////////////////////////////////////
// Arms a timer in each frame, which only an event loop that turns between two frames has fired by the next one.
class CFrameLoopApp final : public Tge::IApp
{
public:

	explicit CFrameLoopApp(bool refuses)
		: m_refuses{ refuses }
	{}

	// Tge::IApp
	bool Initialize() override
	{
		return !m_refuses;
	}

	void Update(float deltaTime) override
	{
		m_isYielding = m_isYielding && gHasMainReturned && (gNumTimers == m_numFrames) && (deltaTime > 0.0f);
		++m_numFrames;

		// None may be pending at the exit, where Emscripten asserts on it.
		if (CanRun())
		{
			emscripten_set_timeout(OnTimer, 0.0, nullptr);
		}
	}

	bool CanRun() const override
	{
		return m_numFrames < NumFrames;
	}

	int Terminate() override
	{
		std::printf("%u frames, %u timers, yielding: %d\n", m_numFrames, gNumTimers, m_isYielding ? 1 : 0);

		int status{ (m_isYielding && (m_numFrames == NumFrames)) ? 0 : 1 };

		if (m_refuses)
		{
			status = (m_numFrames == 0u) ? RefusedStatus : 0;
		}

		return status;
	}
	// ~Tge::IApp

private:

	uint32_t m_numFrames{ 0u };
	bool     m_refuses{ false };
	bool     m_isYielding{ true };
};
} // namespace

//////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
	bool const refuses{ (argc > 1) && (std::string_view{ argv[1] } == "--refuse") };
	int const result{ Tge::RunFrameLoop(std::make_unique<CFrameLoopApp>(refuses)) };

	gHasMainReturned = true;

	return result;
}
