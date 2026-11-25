#include <tge/module/frame_loop.hpp>
#include <tge/module/app.hpp>
#include <emscripten/emscripten.h>
#include <chrono>
#include <utility>

namespace Tge
{
namespace
{
std::unique_ptr<IApp>                 gApp{};
std::chrono::steady_clock::time_point gLastFrame{};

//////////////////////////////////////////////////////////////////////////
void EndRun()
{
	emscripten_cancel_main_loop();

	int const status{ gApp->Terminate() };

	gApp.reset();
	emscripten_force_exit(status);
}

//////////////////////////////////////////////////////////////////////////
void RunFrame()
{
	auto const now{ std::chrono::steady_clock::now() };
	float const deltaTime{ std::chrono::duration<float>(now - gLastFrame).count() };

	gLastFrame = now;
	gApp->Update(deltaTime);

	if (!gApp->CanRun())
	{
		EndRun();
	}
}
} // namespace

//////////////////////////////////////////////////////////////////////////
int RunFrameLoop(std::unique_ptr<IApp> pApp)
{
	gApp = std::move(pApp);

	if (gApp->Initialize() && gApp->CanRun())
	{
		gLastFrame = std::chrono::steady_clock::now();

		// 0: at the browser's own frame rate. false: return to the caller rather than unwind its stack.
		emscripten_set_main_loop(RunFrame, 0, false);
	}
	else
	{
		EndRun();
	}

	return 0;
}
} // namespace Tge
