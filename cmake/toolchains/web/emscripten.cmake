# The web through Emscripten: emsdk's own toolchain does the work, this file adds what every Tge web build shares.
# TGE_EMSDK_PATH names the emsdk checkout. A cache variable with the environment as fallback, as the Linux
# toolchains do: an IDE applies a preset's environment to the build step, never to the configure.
if(NOT TGE_EMSDK_PATH AND DEFINED ENV{EMSDK})
	set(TGE_EMSDK_PATH "$ENV{EMSDK}" CACHE PATH "emsdk checkout")
endif()

set(TGE_WEB_THREADS OFF CACHE BOOL "Build the threaded web variant; the page must then be served cross-origin isolated")

# try_compile re-runs this file in a scratch project that sees the environment but not the cache.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES TGE_EMSDK_PATH TGE_WEB_THREADS)

if(NOT TGE_EMSDK_PATH)
	message(FATAL_ERROR "Set TGE_EMSDK_PATH (or EMSDK in the environment) to an emsdk checkout")
endif()

include("${TGE_EMSDK_PATH}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

# Emscripten.cmake confines package lookups to its sysroot, which would hide every prefix a build is given.
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# A fixed 16 MiB heap suits no real program.
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " -sALLOW_MEMORY_GROWTH=1")

# Every object of a threaded program has to be built for shared memory, dependencies included. Growing shared
# memory is slow in JavaScript unless the browser has growable buffers, so they are used where it does.
if(TGE_WEB_THREADS)
	add_compile_options(-pthread)
	add_link_options(-pthread)
	string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " -sGROWABLE_ARRAYBUFFERS=1")
endif()

message(STATUS "Emscripten from TGE_EMSDK_PATH: ${TGE_EMSDK_PATH} (threads: ${TGE_WEB_THREADS})")
