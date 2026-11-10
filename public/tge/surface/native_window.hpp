#pragma once

#include <cstddef>
#include <cstdint>

// Include before any X11 header: Xlib defines None as a macro.
namespace Tge::Surface
{
// What SNativeWindow's display and window hold.
enum class EWindowSystem : uint32_t
{
	None,
	Xlib,     // Display*, Window
	Wayland,  // wl_display*, wl_surface*
	Win32     // HINSTANCE, HWND
};

// A window as the platform names it, so a presenter builds its surface without knowing the window library.
struct SNativeWindow final
{
	EWindowSystem system{ EWindowSystem::None };
	uintptr_t     display{ 0u };
	uintptr_t     window{ 0u };
};

static_assert(offsetof(SNativeWindow, display) == sizeof(uintptr_t));
static_assert(offsetof(SNativeWindow, window) == 2u * sizeof(uintptr_t));
static_assert(sizeof(SNativeWindow) == 3u * sizeof(uintptr_t));
} // namespace Tge::Surface
