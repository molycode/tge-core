cmake_minimum_required(VERSION 3.28.0)

project(tge-core VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/external.cmake)

if(PROJECT_IS_TOP_LEVEL)
	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

	# No target here declares a module, and leaving this on makes every Clang build need clang-scan-deps,
	# which most distributions ship in a separate package.
	set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

	if(NOT CYGWIN)
		set(CMAKE_CXX_EXTENSIONS OFF)
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		include(cmake/compilers/msvc.cmake)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
		if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
			include(cmake/compilers/clang-cl.cmake)
		elseif(APPLE)
			include(cmake/compilers/apple-clang.cmake)
		else()
			include(cmake/compilers/clang.cmake)
		endif()
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		include(cmake/compilers/gcc.cmake)
	endif()

	set(CMAKE_CONFIGURATION_TYPES Debug Release)
endif() # PROJECT_IS_TOP_LEVEL

# Options
option(TGE_ENABLE_LOGGING "Enable logging system" ON)
option(TGE_ENABLE_MEMORY_TRACKING "Enable memory tracking system" ON)
# rpmalloc maps 256 MiB spans, which a wasm32 heap cannot host; the web keeps Emscripten's allocator.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	set(tgeGlobalAllocatorDefault OFF)
else()
	set(tgeGlobalAllocatorDefault ON)
endif()

option(TGE_ENABLE_GLOBAL_ALLOCATOR "rpmalloc is the heap (global new and delete); OFF leaves rpmalloc out of the build" ${tgeGlobalAllocatorDefault})

if(TGE_ENABLE_GLOBAL_ALLOCATOR AND CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	message(FATAL_ERROR "TGE_ENABLE_GLOBAL_ALLOCATOR cannot be on for the web: rpmalloc's 256 MiB spans do not fit a wasm32 heap")
endif()

option(TGE_CORE_INSTALL "Generate install and export rules for tge-core" ${PROJECT_IS_TOP_LEVEL})
option(TGE_CORE_TESTING "Provide Tge::Testing, the harness every module's tests sit on" OFF)

# OFF by default where every module repository defaults its own suite ON: core vendors no googletest, so a
# bare standalone configure has no GTest to find. Whoever supplies one turns this on.
option(TGE_CORE_BUILD_TESTS "Build tge-core's own test suite (needs GTest 1.14 on the prefix path)" OFF)

# Off for the same reason, and separate from the suite: a benchmark is an instrument, not a gate, so a
# consumer that wants the proof does not thereby want the measurement.
option(TGE_CORE_BUILD_BENCHMARKS "Build tge-core's micro-benchmarks (needs Google Benchmark on the prefix path)" OFF)

# Deciding this only at top level left a vendored Debug build with the flag off while the installed Debug
# package had it on -- one header, compiled two ways. A parent that has already chosen wins.
if(NOT DEFINED TGE_DEBUG_ENABLED)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(TGE_DEBUG_ENABLED ON)
	else()
		set(TGE_DEBUG_ENABLED OFF)
	endif()
endif()

if(PROJECT_IS_TOP_LEVEL)
	if(TGE_DEBUG_ENABLED)
		message(STATUS "[TGE] Debug features ENABLED")
	else()
		message(STATUS "[TGE] Debug features DISABLED")
	endif()

	if(TGE_ENABLE_LOGGING)
		message(STATUS "[TGE] Logging ENABLED")
	else()
		message(STATUS "[TGE] Logging DISABLED")
	endif()

	if(TGE_ENABLE_MEMORY_TRACKING)
		message(STATUS "[TGE] Memory tracking ENABLED")
	else()
		message(STATUS "[TGE] Memory tracking DISABLED")
	endif()

	message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
endif() # PROJECT_IS_TOP_LEVEL

# External dependencies
if(TGE_ENABLE_GLOBAL_ALLOCATOR)
	include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/rpmalloc.cmake)
	tge_suppress_external_warnings(rpmalloc)
endif()

# ============================================================================
# TgeBase (INTERFACE - header-only base utilities)
# ============================================================================
file(GLOB TgeBase_HEADERS CONFIGURE_DEPENDS
	public/tge/*.hpp
)

add_library(TgeBase INTERFACE)
add_library(Tge::Base ALIAS TgeBase)

target_sources(TgeBase PRIVATE ${TgeBase_HEADERS})

target_include_directories(TgeBase INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(TGE_DEBUG_ENABLED)
	target_compile_definitions(TgeBase INTERFACE TGE_DEBUG_ENABLED)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform.cmake)

# ============================================================================
# TgeLogging (STATIC - logging system)
# ============================================================================
file(GLOB TgeLogging_FILES CONFIGURE_DEPENDS
	private/logging/*.cpp
	private/logging/*.hpp
	public/tge/logging/*.hpp
)

add_library(TgeLogging STATIC ${TgeLogging_FILES})
add_library(Tge::Logging ALIAS TgeLogging)

target_link_libraries(TgeLogging
	PUBLIC Tge::Base
)

target_include_directories(TgeLogging
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

if(TGE_ENABLE_LOGGING)
	target_compile_definitions(TgeLogging PUBLIC TGE_LOGGING_ENABLED)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(TgeLogging PRIVATE private/logging/windows/console.cpp private/logging/windows/local_time.cpp)
elseif(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Emscripten)$")
	target_sources(TgeLogging PRIVATE private/logging/posix/console.cpp private/logging/posix/local_time.cpp)
else()
	message(FATAL_ERROR "TgeLogging: no console or local time for ${CMAKE_SYSTEM_NAME}")
endif()

# Copy logging config to build output
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/private/logging/logging.cfg.in
	${CMAKE_BINARY_DIR}/configs/logging.cfg
	COPYONLY
)

# ============================================================================
# TgeMemory (STATIC - memory management with rpmalloc)
# ============================================================================
file(GLOB TgeMemory_FILES CONFIGURE_DEPENDS
	private/memory/*.cpp
	private/memory/*.hpp
	public/tge/memory/*.hpp
)

add_library(TgeMemory STATIC ${TgeMemory_FILES})
add_library(Tge::Memory ALIAS TgeMemory)

target_link_libraries(TgeMemory
	PUBLIC Tge::Base
	PRIVATE Tge::Logging
)

target_include_directories(TgeMemory
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

if(TGE_ENABLE_MEMORY_TRACKING)
	target_compile_definitions(TgeMemory PUBLIC TGE_MEMORY_TRACKING_ENABLED)
endif()

# Off for sanitizers, valgrind, heaptrack and massif: they follow malloc, and rpmalloc takes its pages from mmap.
if(TGE_ENABLE_GLOBAL_ALLOCATOR)
	target_link_libraries(TgeMemory PRIVATE rpmalloc)
	target_compile_definitions(TgeMemory PUBLIC TGE_GLOBAL_ALLOCATOR_ENABLED)
endif()

# ============================================================================
# TgeMath (STATIC - math library)
# ============================================================================
file(GLOB TgeMath_FILES CONFIGURE_DEPENDS
	private/math/*.cpp
	private/math/*.hpp
	public/tge/math/*.hpp
)

add_library(TgeMath STATIC ${TgeMath_FILES})
add_library(Tge::Math ALIAS TgeMath)

target_link_libraries(TgeMath
	PUBLIC Tge::Base
)

target_include_directories(TgeMath
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/glm>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ============================================================================
# TgeThreading (STATIC - job system, thread pool and event loop)
# ============================================================================
file(GLOB TgeThreading_FILES CONFIGURE_DEPENDS
	private/threading/*.cpp
	private/threading/*.hpp
	public/tge/threading/*.hpp
)

# The event loop waits on sockets for the net module, which reaches the web through the browser instead.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	list(REMOVE_ITEM TgeThreading_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/private/threading/event_loop.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/private/threading/poller.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/private/threading/poller.hpp
	)
endif()

add_library(TgeThreading STATIC ${TgeThreading_FILES})
add_library(Tge::Threading ALIAS TgeThreading)

target_link_libraries(TgeThreading
	PUBLIC Tge::Base
	PRIVATE Tge::Memory Tge::Logging $<$<PLATFORM_ID:Windows>:ws2_32>
)

target_include_directories(TgeThreading
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" AND NOT TGE_WEB_THREADS)
	target_sources(TgeThreading PRIVATE private/threading/thread_support/none.cpp)
else()
	target_sources(TgeThreading PRIVATE private/threading/thread_support/threads.cpp)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(TgeThreading PRIVATE private/threading/windows/thread_name.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_sources(TgeThreading PRIVATE private/threading/linux/thread_name.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	target_sources(TgeThreading PRIVATE private/threading/web/thread_name.cpp)
else()
	message(FATAL_ERROR "TgeThreading: no thread naming for ${CMAKE_SYSTEM_NAME}")
endif()

# ============================================================================
# TgeIO (STATIC - files, paths, user folders and folder watches)
# ============================================================================
if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows|Emscripten)$")
	# The web has whole-file reads, atomic writes and user folders: nothing built for it uses the rest.
	if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		set(TgeIO_FILES
			private/io/default_user_folder.hpp
			private/io/environment_path.hpp
			private/io/flush_user_folders.hpp
			private/io/get_user_folder.cpp
			private/io/resolve_link.cpp
			private/io/resolve_link.hpp
			public/tge/io/get_user_folder.hpp
			public/tge/io/read_whole_file.hpp
			public/tge/io/user_folder.hpp
			public/tge/io/write_file_atomically.hpp
		)
	else()
		file(GLOB TgeIO_FILES CONFIGURE_DEPENDS
			private/io/*.cpp
			private/io/*.hpp
			public/tge/io/*.hpp
		)
	endif()

	add_library(TgeIO STATIC ${TgeIO_FILES})
	add_library(Tge::IO ALIAS TgeIO)

	target_link_libraries(TgeIO
		PUBLIC Tge::Base
		PRIVATE $<$<PLATFORM_ID:Windows>:shell32> $<$<PLATFORM_ID:Windows>:ole32>
	)

	target_include_directories(TgeIO
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
	)

	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		target_sources(TgeIO PRIVATE
			private/io/native/flush_user_folders.cpp
			private/io/windows/default_user_folder.cpp
			private/io/windows/environment_path.cpp
			private/io/windows/read_whole_file.cpp
			private/io/windows/write_file_atomically.cpp
		)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		target_sources(TgeIO PRIVATE
			private/io/linux/default_user_folder.cpp
			private/io/linux/flush_file.cpp
			private/io/linux/user_folder_error_category.cpp
			private/io/linux/user_folder_error_category.hpp
			private/io/native/flush_user_folders.cpp
			private/io/posix/environment_path.cpp
			private/io/posix/flush_file.hpp
			private/io/posix/read_whole_file.cpp
			private/io/posix/write_file_atomically.cpp
		)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		target_sources(TgeIO PRIVATE
			private/io/posix/environment_path.cpp
			private/io/posix/flush_file.hpp
			private/io/posix/read_whole_file.cpp
			private/io/posix/write_file_atomically.cpp
			private/io/web/default_user_folder.cpp
			private/io/web/finish_user_folders.hpp
			private/io/web/flush_file.cpp
			private/io/web/flush_user_folders.cpp
			private/io/web/user_folders.hpp
			private/io/web/user_folders.js
		)

		target_link_libraries(TgeIO PRIVATE Tge::Logging)

		target_link_options(TgeIO INTERFACE
			-lidbfs.js
			$<BUILD_INTERFACE:--js-library=${CMAKE_CURRENT_SOURCE_DIR}/private/io/web/user_folders.js>
			$<INSTALL_INTERFACE:--js-library=$<INSTALL_PREFIX>/${CMAKE_INSTALL_LIBDIR}/cmake/TgeCore/user_folders.js>
		)

		set_property(TARGET TgeIO PROPERTY INTERFACE_LINK_DEPENDS
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/private/io/web/user_folders.js>
		)
	else()
		message(FATAL_ERROR "TgeIO: no files or user folders for ${CMAKE_SYSTEM_NAME}")
	endif()
endif()

# ============================================================================
# TgeProcess (STATIC - child processes: start them and see them end)
# ============================================================================
# Linux and Windows only: anywhere else its sources stop at #error, which would fail every build of all.
if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows)$")
	file(GLOB TgeProcess_FILES CONFIGURE_DEPENDS
		private/process/*.cpp
		private/process/*.hpp
		public/tge/process/*.hpp
	)

	add_library(TgeProcess STATIC ${TgeProcess_FILES})
	add_library(Tge::Process ALIAS TgeProcess)

	target_link_libraries(TgeProcess
		PUBLIC Tge::Base
	)

	target_include_directories(TgeProcess
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
	)
endif()

# ============================================================================
# TgeCommand (STATIC - console command registration and dispatch)
# ============================================================================
file(GLOB TgeCommand_FILES CONFIGURE_DEPENDS
	private/command/*.cpp
	private/command/*.hpp
	public/tge/command/*.hpp
)

add_library(TgeCommand STATIC ${TgeCommand_FILES})
add_library(Tge::Command ALIAS TgeCommand)

target_link_libraries(TgeCommand
	PUBLIC Tge::Base
	PRIVATE Tge::Memory Tge::Logging
)

target_include_directories(TgeCommand
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# ============================================================================
# TgeInit (STATIC - initialization/termination)
# ============================================================================
file(GLOB TgeInit_FILES CONFIGURE_DEPENDS
	private/init/*.cpp
	private/init/*.hpp
	public/tge/init/*.hpp
)

add_library(TgeInit STATIC ${TgeInit_FILES})
add_library(Tge::Init ALIAS TgeInit)

target_link_libraries(TgeInit
	PUBLIC Tge::Base
	PRIVATE Tge::Memory Tge::Threading Tge::Logging Tge::Command Tge::IO
)

target_include_directories(TgeInit
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# ============================================================================
# TgeProfiling (INTERFACE - header-only profiling vocabulary)
# ============================================================================
file(GLOB TgeProfiling_HEADERS CONFIGURE_DEPENDS
	public/tge/profiling/*.hpp
)

add_library(TgeProfiling INTERFACE)
add_library(Tge::Profiling ALIAS TgeProfiling)

target_sources(TgeProfiling PRIVATE ${TgeProfiling_HEADERS})

target_link_libraries(TgeProfiling
	INTERFACE Tge::Base Tge::Memory Tge::Threading
)

# The backend is partnered at compile time, so a consumer that never adds a Tracy client compiles the markers
# away to nothing rather than carrying a runtime switch.
if(TARGET Tracy::TracyClient)
	target_link_libraries(TgeProfiling INTERFACE Tracy::TracyClient)
	target_compile_definitions(TgeProfiling INTERFACE TGE_PROFILE_TRACY)
	message(STATUS "[TGE-CORE] Profiling backend: Tracy")
endif()

# ============================================================================
# tge/config.hpp - the flags, in a header rather than only on the command line
# ============================================================================
# Generated here, after every flag above has been decided (TGE_PROFILE_TRACY is the last, and depends on
# whether a Tracy client target exists). See config.hpp.in for why the command line alone is not enough.
set(TGE_MEMORY_TRACKING_ENABLED ${TGE_ENABLE_MEMORY_TRACKING})
set(TGE_GLOBAL_ALLOCATOR_ENABLED ${TGE_ENABLE_GLOBAL_ALLOCATOR})
set(TGE_LOGGING_ENABLED ${TGE_ENABLE_LOGGING})

if(TARGET Tracy::TracyClient)
	set(TGE_PROFILE_TRACY ON)
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/public/tge/config.hpp.in
	${CMAKE_CURRENT_BINARY_DIR}/generated/tge/config.hpp
	@ONLY
)

target_include_directories(TgeBase INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

# ============================================================================
# TgeModule (STATIC - the module contract every module compiles against)
# ============================================================================
file(GLOB TgeModule_HEADERS CONFIGURE_DEPENDS
	public/tge/module/*.hpp
)

# Listed rather than globbed: the driver's sources sit in the same directory and must NOT land here -- a
# module declares a dependency on Core's identity without linking anything that can drive a module.
add_library(TgeModule STATIC
	${TgeModule_HEADERS}
	private/module/module_id.cpp
)
add_library(Tge::Module ALIAS TgeModule)

target_link_libraries(TgeModule
	PUBLIC Tge::Base
)

# ============================================================================
# TgeLifecycle (STATIC - module ordering, phase dispatch and the frame loop, for a composition root)
# ============================================================================
add_library(TgeLifecycle STATIC
	private/module/module.hpp
	private/module/module.cpp
	private/module/module_graph.hpp
	private/module/module_graph.cpp
	private/module/runtime.hpp
	private/module/runtime.cpp
)
add_library(Tge::Lifecycle ALIAS TgeLifecycle)

target_link_libraries(TgeLifecycle
	PUBLIC Tge::Module
	PRIVATE Tge::Logging Tge::Init Tge::Profiling
)

target_include_directories(TgeLifecycle
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows)$")
	target_sources(TgeLifecycle PRIVATE private/module/native/frame_loop.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	target_sources(TgeLifecycle PRIVATE private/module/web/frame_loop.cpp)
	target_link_libraries(TgeLifecycle PRIVATE Tge::IO)
else()
	message(FATAL_ERROR "TgeLifecycle: no frame loop for ${CMAKE_SYSTEM_NAME}")
endif()

# ============================================================================
# TgeEventsPublic (STATIC - the event interface, its globals and the module's identity)
# ============================================================================
file(GLOB TgeEvents_HEADERS CONFIGURE_DEPENDS
	public/tge/events/*.hpp
)

# Listed rather than globbed: the dispatcher's sources sit in the same directory and must NOT land here -- a
# module emits and subscribes without linking anything that can drive the queue.
add_library(TgeEventsPublic STATIC
	${TgeEvents_HEADERS}
	private/events/globals.cpp
	private/events/module_id.cpp
)
add_library(Tge::EventsPublic ALIAS TgeEventsPublic)

target_link_libraries(TgeEventsPublic
	PUBLIC Tge::Module
)

# ============================================================================
# TgeEvents (STATIC - the dispatcher and the module that drains its queue)
# ============================================================================
add_library(TgeEvents STATIC
	private/events/system.hpp
	private/events/system.cpp
	private/events/module.hpp
	private/events/module.cpp
)
add_library(Tge::Events ALIAS TgeEvents)

target_link_libraries(TgeEvents
	PUBLIC Tge::EventsPublic
)

# ============================================================================
# TgeAssetsPublic (STATIC - the asset-provider contract, its settings and the provider's identity)
# ============================================================================
file(GLOB TgeAssets_HEADERS CONFIGURE_DEPENDS
	public/tge/assets/*.hpp
)

# No implementation here: the module that provides the contract reports gModuleId as its own identity.
add_library(TgeAssetsPublic STATIC
	${TgeAssets_HEADERS}
	private/assets/globals.cpp
	private/assets/module_id.cpp
)
add_library(Tge::AssetsPublic ALIAS TgeAssetsPublic)

target_link_libraries(TgeAssetsPublic
	PUBLIC
		Tge::Module
		Tge::Model
)

# ============================================================================
# TgeGeometry (INTERFACE - header-only geometry vocabulary)
# ============================================================================
file(GLOB TgeGeometry_HEADERS CONFIGURE_DEPENDS
	public/tge/geometry/*.hpp
)

add_library(TgeGeometry INTERFACE)
add_library(Tge::Geometry ALIAS TgeGeometry)

target_sources(TgeGeometry PRIVATE ${TgeGeometry_HEADERS})

target_link_libraries(TgeGeometry
	INTERFACE Tge::Math
)

# ============================================================================
# TgeMaterial (INTERFACE - header-only material vocabulary)
# ============================================================================
file(GLOB TgeMaterial_HEADERS CONFIGURE_DEPENDS
	public/tge/material/*.hpp
)

add_library(TgeMaterial INTERFACE)
add_library(Tge::Material ALIAS TgeMaterial)

target_sources(TgeMaterial PRIVATE ${TgeMaterial_HEADERS})

target_link_libraries(TgeMaterial
	INTERFACE Tge::Math
)

# ============================================================================
# TgeEntity (INTERFACE - header-only entity-handle vocabulary)
# ============================================================================
file(GLOB TgeEntity_HEADERS CONFIGURE_DEPENDS
	public/tge/entity/*.hpp
)

add_library(TgeEntity INTERFACE)
add_library(Tge::Entity ALIAS TgeEntity)

target_sources(TgeEntity PRIVATE ${TgeEntity_HEADERS})

target_link_libraries(TgeEntity
	INTERFACE Tge::Base
)

# ============================================================================
# TgeLight (INTERFACE - header-only light vocabulary)
# ============================================================================
file(GLOB TgeLight_HEADERS CONFIGURE_DEPENDS
	public/tge/light/*.hpp
)

add_library(TgeLight INTERFACE)
add_library(Tge::Light ALIAS TgeLight)

target_sources(TgeLight PRIVATE ${TgeLight_HEADERS})

target_link_libraries(TgeLight
	INTERFACE Tge::Math
)

# ============================================================================
# TgeProbe (INTERFACE - header-only probe adjustment vocabulary)
# ============================================================================
file(GLOB TgeProbe_HEADERS CONFIGURE_DEPENDS
	public/tge/probe/*.hpp
)

add_library(TgeProbe INTERFACE)
add_library(Tge::Probe ALIAS TgeProbe)

target_sources(TgeProbe PRIVATE ${TgeProbe_HEADERS})

target_link_libraries(TgeProbe
	INTERFACE Tge::Math
)

# ============================================================================
# TgeParticles (INTERFACE - header-only particle emitter vocabulary)
# ============================================================================
file(GLOB TgeParticles_HEADERS CONFIGURE_DEPENDS
	public/tge/particles/*.hpp
)

add_library(TgeParticles INTERFACE)
add_library(Tge::Particles ALIAS TgeParticles)

target_sources(TgeParticles PRIVATE ${TgeParticles_HEADERS})

target_link_libraries(TgeParticles
	INTERFACE Tge::Base
)

# ============================================================================
# TgeExposure (INTERFACE - header-only exposure vocabulary)
# ============================================================================
file(GLOB TgeExposure_HEADERS CONFIGURE_DEPENDS
	public/tge/exposure/*.hpp
)

add_library(TgeExposure INTERFACE)
add_library(Tge::Exposure ALIAS TgeExposure)

target_sources(TgeExposure PRIVATE ${TgeExposure_HEADERS})

target_link_libraries(TgeExposure
	INTERFACE Tge::Math
)

# ============================================================================
# TgeSurface (INTERFACE - header-only native-window vocabulary)
# ============================================================================
file(GLOB TgeSurface_HEADERS CONFIGURE_DEPENDS
	public/tge/surface/*.hpp
)

add_library(TgeSurface INTERFACE)
add_library(Tge::Surface ALIAS TgeSurface)

target_sources(TgeSurface PRIVATE ${TgeSurface_HEADERS})

target_link_libraries(TgeSurface
	INTERFACE Tge::Base
)

# ============================================================================
# TgeUiDraw (INTERFACE - header-only UI draw-list vocabulary)
# ============================================================================
file(GLOB TgeUiDraw_HEADERS CONFIGURE_DEPENDS
	public/tge/ui_draw/*.hpp
)

add_library(TgeUiDraw INTERFACE)
add_library(Tge::UiDraw ALIAS TgeUiDraw)

target_sources(TgeUiDraw PRIVATE ${TgeUiDraw_HEADERS})

target_link_libraries(TgeUiDraw
	INTERFACE Tge::Math
)

# ============================================================================
# TgeModel (INTERFACE - header-only model, mesh, texture and animation-clip vocabulary)
# ============================================================================
file(GLOB TgeModel_HEADERS CONFIGURE_DEPENDS
	public/tge/model/*.hpp
)

add_library(TgeModel INTERFACE)
add_library(Tge::Model ALIAS TgeModel)

target_sources(TgeModel PRIVATE ${TgeModel_HEADERS})

target_link_libraries(TgeModel
	INTERFACE
		Tge::Math
		Tge::Geometry
		Tge::Light
		Tge::Probe
		Tge::Particles
		Tge::Material
)

# ============================================================================
# TgeCore (INTERFACE - convenience all-in-one target)
# ============================================================================
add_library(TgeCore INTERFACE)
add_library(Tge::Core ALIAS TgeCore)

target_link_libraries(TgeCore INTERFACE
	Tge::Base
	Tge::Logging
	Tge::Memory
	Tge::Math
	Tge::Threading
	Tge::Init
	Tge::Command
	Tge::Profiling
	Tge::Module
	Tge::Geometry
	Tge::Material
	Tge::Entity
	Tge::Light
	Tge::Probe
	Tge::Particles
	Tge::Exposure
	Tge::Surface
	Tge::UiDraw
	Tge::Model
)

if(TARGET TgeIO)
	target_link_libraries(TgeCore INTERFACE Tge::IO)
endif()

# TgeLifecycle is deliberately absent: it drives modules, and only a composition root does that.
# TgeEvents and TgeEventsPublic likewise: QueueEmit's backlog is drained by a frame phase, so a consumer that
# runs no frame loop would acquire a queue nothing empties. Both are named explicitly by whoever wants them.
# TgeProcess too: it is built on Linux and Windows only, which the umbrella cannot assume.

# ============================================================================
# TgeTesting (INTERFACE - the harness every module's tests sit on)
# ============================================================================
# Absent from TgeCore for the same reason as TgeLifecycle: only a test binary links a test harness.
if(TGE_CORE_TESTING)
	# Both branches announce themselves: a parent that entered googletest first wins this silently, and the
	# arm not taken is the one nobody can see from the log afterwards.
	if(NOT TARGET GTest::gtest)
		find_package(GTest 1.14 REQUIRED)
		message(STATUS "[TgeCore] GTest: package ${GTest_VERSION} from ${GTest_DIR}")
	else()
		message(STATUS "[TgeCore] GTest: already provided")
	endif()

	file(GLOB TgeTesting_HEADERS CONFIGURE_DEPENDS
		public/tge/testing/*.hpp
	)

	add_library(TgeTesting INTERFACE)
	add_library(Tge::Testing ALIAS TgeTesting)

	target_sources(TgeTesting PRIVATE ${TgeTesting_HEADERS})

	target_link_libraries(TgeTesting
		INTERFACE Tge::Base Tge::Logging GTest::gtest
	)
endif()

if(TGE_CORE_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

if(TGE_CORE_BUILD_BENCHMARKS)
	add_subdirectory(benchmarks)
endif()

if(TGE_CORE_INSTALL)
	include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/install.cmake)
endif()
