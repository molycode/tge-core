# CMake-side platform awareness.
# Platform-specific build settings (link libraries, flags, etc.) go here.
# C++ platform detection is handled independently in <tge/platform.hpp>.

if(TGE_PLATFORM_WINDOWS)
	message(STATUS "[TGE] Platform: Windows")
elseif(TGE_PLATFORM_LINUX)
	message(STATUS "[TGE] Platform: Linux")
elseif(TGE_PLATFORM_MACOS)
	message(STATUS "[TGE] Platform: macOS")
elseif(TGE_PLATFORM_IOS)
	message(STATUS "[TGE] Platform: iOS")
elseif(TGE_PLATFORM_ANDROID)
	message(STATUS "[TGE] Platform: Android")
else()
	message(WARNING "[TGE] Platform '${CMAKE_SYSTEM_NAME}' has no TGE_PLATFORM_* variable set. "
		"Platform-specific build settings will not be applied. "
		"Add a TGE_PLATFORM_<X> entry to your toolchain file and cmake/platform.cmake.")
endif()
