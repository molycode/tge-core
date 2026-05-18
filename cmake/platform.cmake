# CMake-side platform awareness.
# Platform-specific build settings (link libraries, flags, etc.) go here.
# C++ platform detection is handled independently in <tge/platform.hpp>.

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	message(STATUS "[TGE] Platform: Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	message(STATUS "[TGE] Platform: Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	message(STATUS "[TGE] Platform: macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
	message(STATUS "[TGE] Platform: iOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
	message(STATUS "[TGE] Platform: Android")
else()
	message(WARNING "[TGE] Platform '${CMAKE_SYSTEM_NAME}' has no build settings configured. "
		"Platform-specific settings will not be applied. "
		"Add a corresponding block to cmake/platform.cmake.")
endif()
