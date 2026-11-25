#include <gtest/gtest.h>
#include <tge/module/app.hpp>
#include <tge/module/frame_loop.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace
{
constexpr int ExitStatus{ 7 };
constexpr std::chrono::milliseconds FrameDuration{ 5 };

// What the app did, outliving it: the frame loop owns the app.
struct SRecord final
{
	std::string calls{};
	float       totalDelta{ 0.0f };
};

class CFakeApp final : public Tge::IApp
{
public:

	CFakeApp(SRecord& record, uint32_t numFrames, bool canInitialize)
		: m_record{ record }
		, m_numFramesWanted{ numFrames }
		, m_canInitialize{ canInitialize }
	{}

	~CFakeApp() override
	{
		m_record.calls += "destroyed";
	}

	// Tge::IApp
	bool Initialize() override
	{
		m_record.calls += "initialize ";

		return m_canInitialize;
	}

	void Update(float deltaTime) override
	{
		m_record.calls += "update ";
		m_record.totalDelta += deltaTime;
		++m_numFrames;

		std::this_thread::sleep_for(FrameDuration);
	}

	bool CanRun() const override
	{
		return m_numFrames < m_numFramesWanted;
	}

	int Terminate() override
	{
		m_record.calls += "terminate ";

		return ExitStatus;
	}
	// ~Tge::IApp

private:

	SRecord& m_record;
	uint32_t m_numFramesWanted{ 0u };
	uint32_t m_numFrames{ 0u };
	bool     m_canInitialize{ true };
};
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST(FrameLoop, RunsEveryFrameAndEnds)
{
	SRecord record{};

	EXPECT_EQ(Tge::RunFrameLoop(std::make_unique<CFakeApp>(record, 3u, true)), ExitStatus);
	EXPECT_EQ(record.calls, "initialize update update update terminate destroyed");
}

//////////////////////////////////////////////////////////////////////////
TEST(FrameLoop, RunsNoFrameWhenTheAppCannotRun)
{
	SRecord record{};

	EXPECT_EQ(Tge::RunFrameLoop(std::make_unique<CFakeApp>(record, 0u, true)), ExitStatus);
	EXPECT_EQ(record.calls, "initialize terminate destroyed");
}

//////////////////////////////////////////////////////////////////////////
TEST(FrameLoop, RefusedAppIsTerminatedWithoutAFrame)
{
	SRecord record{};

	EXPECT_EQ(Tge::RunFrameLoop(std::make_unique<CFakeApp>(record, 3u, false)), ExitStatus);
	EXPECT_EQ(record.calls, "initialize terminate destroyed");
}

//////////////////////////////////////////////////////////////////////////
// Every frame but the first is handed the one before it, so the deltas hold four of the five sleeps and no time twice.
TEST(FrameLoop, DeltasPartitionTheTime)
{
	SRecord record{};
	auto const start{ std::chrono::steady_clock::now() };

	Tge::RunFrameLoop(std::make_unique<CFakeApp>(record, 5u, true));

	float const elapsed{ std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() };

	EXPECT_GE(record.totalDelta, std::chrono::duration<float>(FrameDuration * 4).count());
	EXPECT_LE(record.totalDelta, elapsed);
}
