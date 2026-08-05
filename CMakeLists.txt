cmake_minimum_required(VERSION 4.3.0)

project(tge-core VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/external.cmake)

if(PROJECT_IS_TOP_LEVEL)
	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

option(TGE_CORE_INSTALL "Generate install and export rules for tge-core" ${PROJECT_IS_TOP_LEVEL})
option(TGE_CORE_TESTING "Provide Tge::Testing, the harness every module's tests sit on" OFF)

# OFF by default where every module repository defaults its own suite ON: core vendors no googletest, so a
# bare standalone configure has no GTest to find. Whoever supplies one turns this on.
option(TGE_CORE_BUILD_TESTS "Build tge-core's own test suite (needs GTest 1.14 on the prefix path)" OFF)

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
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/rpmalloc.cmake)
tge_suppress_external_warnings(rpmalloc)

# ============================================================================
# TgeBase (INTERFACE - header-only base utilities)
# ============================================================================
file(GLOB TgeBase_HEADERS CONFIGURE_DEPENDS
	code/public/tge/*.hpp
)

add_library(TgeBase INTERFACE)
add_library(Tge::Base ALIAS TgeBase)

target_sources(TgeBase PRIVATE ${TgeBase_HEADERS})

target_include_directories(TgeBase INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/code/public>
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
	code/private/logging/*.cpp
	code/private/logging/*.hpp
	code/public/tge/logging/*.hpp
)

add_library(TgeLogging STATIC ${TgeLogging_FILES})
add_library(Tge::Logging ALIAS TgeLogging)

target_link_libraries(TgeLogging
	PUBLIC Tge::Base
)

target_include_directories(TgeLogging
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

if(TGE_ENABLE_LOGGING)
	target_compile_definitions(TgeLogging PUBLIC TGE_LOGGING_ENABLED)
endif()

# Copy logging config to build output
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/code/private/logging/logging.cfg.in
	${CMAKE_BINARY_DIR}/configs/logging.cfg
	COPYONLY
)

# ============================================================================
# TgeMemory (STATIC - memory management with rpmalloc)
# ============================================================================
file(GLOB TgeMemory_FILES CONFIGURE_DEPENDS
	code/private/memory/*.cpp
	code/private/memory/*.hpp
	code/public/tge/memory/*.hpp
)

add_library(TgeMemory STATIC ${TgeMemory_FILES})
add_library(Tge::Memory ALIAS TgeMemory)

target_link_libraries(TgeMemory
	PUBLIC Tge::Base
	PRIVATE rpmalloc Tge::Logging
)

target_include_directories(TgeMemory
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

if(TGE_ENABLE_MEMORY_TRACKING)
	target_compile_definitions(TgeMemory PUBLIC TGE_MEMORY_TRACKING_ENABLED)
endif()

# ============================================================================
# TgeMath (STATIC - math library)
# ============================================================================
file(GLOB TgeMath_FILES CONFIGURE_DEPENDS
	code/private/math/*.cpp
	code/private/math/*.hpp
	code/public/tge/math/*.hpp
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
# TgeThreading (STATIC - job system and thread pool)
# ============================================================================
file(GLOB TgeThreading_FILES CONFIGURE_DEPENDS
	code/private/threading/*.cpp
	code/private/threading/*.hpp
	code/public/tge/threading/*.hpp
)

add_library(TgeThreading STATIC ${TgeThreading_FILES})
add_library(Tge::Threading ALIAS TgeThreading)

target_link_libraries(TgeThreading
	PUBLIC Tge::Base
	PRIVATE rpmalloc Tge::Logging
)

target_include_directories(TgeThreading
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeIO (STATIC - file and path operations)
# ============================================================================
file(GLOB TgeIO_FILES CONFIGURE_DEPENDS
	code/private/io/*.cpp
	code/private/io/*.hpp
	code/public/tge/io/*.hpp
)

add_library(TgeIO STATIC ${TgeIO_FILES})
add_library(Tge::IO ALIAS TgeIO)

target_link_libraries(TgeIO
	PUBLIC Tge::Base
)

target_include_directories(TgeIO
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeCommand (STATIC - console command registration and dispatch)
# ============================================================================
file(GLOB TgeCommand_FILES CONFIGURE_DEPENDS
	code/private/command/*.cpp
	code/private/command/*.hpp
	code/public/tge/command/*.hpp
)

add_library(TgeCommand STATIC ${TgeCommand_FILES})
add_library(Tge::Command ALIAS TgeCommand)

target_link_libraries(TgeCommand
	PUBLIC Tge::Base
	PRIVATE Tge::Memory Tge::Logging
)

target_include_directories(TgeCommand
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeInit (STATIC - initialization/termination)
# ============================================================================
file(GLOB TgeInit_FILES CONFIGURE_DEPENDS
	code/private/init/*.cpp
	code/private/init/*.hpp
	code/public/tge/init/*.hpp
)

add_library(TgeInit STATIC ${TgeInit_FILES})
add_library(Tge::Init ALIAS TgeInit)

target_link_libraries(TgeInit
	PUBLIC Tge::Base
	PRIVATE Tge::Memory Tge::Threading Tge::Logging Tge::Command
)

target_include_directories(TgeInit
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeProfiling (INTERFACE - header-only profiling vocabulary)
# ============================================================================
file(GLOB TgeProfiling_HEADERS CONFIGURE_DEPENDS
	code/public/tge/profiling/*.hpp
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
set(TGE_LOGGING_ENABLED ${TGE_ENABLE_LOGGING})

if(TARGET Tracy::TracyClient)
	set(TGE_PROFILE_TRACY ON)
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/code/public/tge/config.hpp.in
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
	code/public/tge/module/*.hpp
)

# Listed rather than globbed: the driver's sources sit in the same directory and must NOT land here -- a
# module declares a dependency on Core's identity without linking anything that can drive a module.
add_library(TgeModule STATIC
	${TgeModule_HEADERS}
	code/private/module/module_id.cpp
)
add_library(Tge::Module ALIAS TgeModule)

target_link_libraries(TgeModule
	PUBLIC Tge::Base
)

# ============================================================================
# TgeLifecycle (STATIC - module ordering and phase dispatch, for a composition root)
# ============================================================================
add_library(TgeLifecycle STATIC
	code/private/module/module.hpp
	code/private/module/module.cpp
	code/private/module/module_graph.hpp
	code/private/module/module_graph.cpp
	code/private/module/runtime.hpp
	code/private/module/runtime.cpp
)
add_library(Tge::Lifecycle ALIAS TgeLifecycle)

target_link_libraries(TgeLifecycle
	PUBLIC Tge::Module
	PRIVATE Tge::Logging Tge::Init Tge::Profiling
)

target_include_directories(TgeLifecycle
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeEventsPublic (STATIC - the event interface, its globals and the module's identity)
# ============================================================================
file(GLOB TgeEvents_HEADERS CONFIGURE_DEPENDS
	code/public/tge/events/*.hpp
)

# Listed rather than globbed: the dispatcher's sources sit in the same directory and must NOT land here -- a
# module emits and subscribes without linking anything that can drive the queue.
add_library(TgeEventsPublic STATIC
	${TgeEvents_HEADERS}
	code/private/events/globals.cpp
	code/private/events/module_id.cpp
)
add_library(Tge::EventsPublic ALIAS TgeEventsPublic)

target_link_libraries(TgeEventsPublic
	PUBLIC Tge::Module
)

# ============================================================================
# TgeEvents (STATIC - the dispatcher and the module that drains its queue)
# ============================================================================
add_library(TgeEvents STATIC
	code/private/events/system.hpp
	code/private/events/system.cpp
	code/private/events/module.hpp
	code/private/events/module.cpp
)
add_library(Tge::Events ALIAS TgeEvents)

target_link_libraries(TgeEvents
	PUBLIC Tge::EventsPublic
)

# ============================================================================
# TgeGeometry (INTERFACE - header-only geometry vocabulary)
# ============================================================================
file(GLOB TgeGeometry_HEADERS CONFIGURE_DEPENDS
	code/public/tge/geometry/*.hpp
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
	code/public/tge/material/*.hpp
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
	code/public/tge/entity/*.hpp
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
	code/public/tge/light/*.hpp
)

add_library(TgeLight INTERFACE)
add_library(Tge::Light ALIAS TgeLight)

target_sources(TgeLight PRIVATE ${TgeLight_HEADERS})

target_link_libraries(TgeLight
	INTERFACE Tge::Math
)

# ============================================================================
# TgeExposure (INTERFACE - header-only exposure vocabulary)
# ============================================================================
file(GLOB TgeExposure_HEADERS CONFIGURE_DEPENDS
	code/public/tge/exposure/*.hpp
)

add_library(TgeExposure INTERFACE)
add_library(Tge::Exposure ALIAS TgeExposure)

target_sources(TgeExposure PRIVATE ${TgeExposure_HEADERS})

target_link_libraries(TgeExposure
	INTERFACE Tge::Math
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
	Tge::IO
	Tge::Init
	Tge::Command
	Tge::Profiling
	Tge::Module
	Tge::Geometry
	Tge::Material
	Tge::Entity
	Tge::Light
	Tge::Exposure
)

# TgeLifecycle is deliberately absent: it drives modules, and only a composition root does that.
# TgeEvents and TgeEventsPublic likewise: QueueEmit's backlog is drained by a frame phase, so a consumer that
# runs no frame loop would acquire a queue nothing empties. Both are named explicitly by whoever wants them.

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
		code/public/tge/testing/*.hpp
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

if(TGE_CORE_INSTALL)
	include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/install.cmake)
endif()
