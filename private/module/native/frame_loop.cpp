#include <tge/module/frame_loop.hpp>
#include <tge/module/app.hpp>
#include <chrono>

namespace Tge
{
//////////////////////////////////////////////////////////////////////////
int RunFrameLoop(std::unique_ptr<IApp> pApp)
{
	if (pApp->Initialize())
	{
		auto lastFrame{ std::chrono::steady_clock::now() };

		while (pApp->CanRun())
		{
			auto const now{ std::chrono::steady_clock::now() };
			float const deltaTime{ std::chrono::duration<float>(now - lastFrame).count() };

			lastFrame = now;
			pApp->Update(deltaTime);
		}
	}

	return pApp->Terminate();
}
} // namespace Tge
