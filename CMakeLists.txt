cmake_minimum_required(VERSION 4.3.0)

project(tge-core LANGUAGES C CXX)

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

if(PROJECT_IS_TOP_LEVEL)
	# Debug features in Debug builds
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_compile_definitions(TGE_DEBUG_ENABLED)
		message(STATUS "[TGE] Debug features ENABLED")
	else()
		message(STATUS "[TGE] Debug features DISABLED")
	endif()

	if(TGE_ENABLE_LOGGING)
		add_compile_definitions(TGE_LOGGING_ENABLED)
		message(STATUS "[TGE] Logging ENABLED")
	else()
		message(STATUS "[TGE] Logging DISABLED")
	endif()

	if(TGE_ENABLE_MEMORY_TRACKING)
		add_compile_definitions(TGE_MEMORY_TRACKING_ENABLED)
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

target_sources(TgeBase INTERFACE ${TgeBase_HEADERS})

target_include_directories(TgeBase INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/code/public
)

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

target_link_libraries(TgeLogging
	PUBLIC TgeBase
)

target_include_directories(TgeLogging
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

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

target_link_libraries(TgeMemory
	PUBLIC TgeBase
	PRIVATE rpmalloc TgeLogging
)

target_include_directories(TgeMemory
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeMath (STATIC - math library)
# ============================================================================
file(GLOB TgeMath_FILES CONFIGURE_DEPENDS
	code/private/math/*.cpp
	code/private/math/*.hpp
	code/public/tge/math/*.hpp
)

add_library(TgeMath STATIC ${TgeMath_FILES})

target_link_libraries(TgeMath
	PUBLIC TgeBase
)

target_include_directories(TgeMath
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
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

target_link_libraries(TgeThreading
	PUBLIC TgeBase
	PRIVATE rpmalloc TgeLogging
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

target_link_libraries(TgeIO
	PUBLIC TgeBase
)

target_include_directories(TgeIO
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

target_link_libraries(TgeInit
	PUBLIC TgeBase
	PRIVATE TgeMemory TgeThreading TgeIO TgeLogging
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

target_sources(TgeProfiling INTERFACE ${TgeProfiling_HEADERS})

target_link_libraries(TgeProfiling
	INTERFACE TgeBase TgeMemory TgeThreading
)

# The backend is partnered at compile time, so a consumer that never adds a Tracy client compiles the markers
# away to nothing rather than carrying a runtime switch.
if(TARGET Tracy::TracyClient)
	target_link_libraries(TgeProfiling INTERFACE Tracy::TracyClient)
	target_compile_definitions(TgeProfiling INTERFACE TGE_PROFILE_TRACY)
	message(STATUS "[TGE-CORE] Profiling backend: Tracy")
endif()

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

target_link_libraries(TgeModule
	PUBLIC TgeBase
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

target_link_libraries(TgeLifecycle
	PUBLIC TgeModule
	PRIVATE TgeLogging TgeInit TgeProfiling
)

target_include_directories(TgeLifecycle
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeGeometry (INTERFACE - header-only geometry vocabulary)
# ============================================================================
file(GLOB TgeGeometry_HEADERS CONFIGURE_DEPENDS
	code/public/tge/geometry/*.hpp
)

add_library(TgeGeometry INTERFACE)

target_sources(TgeGeometry INTERFACE ${TgeGeometry_HEADERS})

target_link_libraries(TgeGeometry
	INTERFACE TgeMath
)

# ============================================================================
# TgeMaterial (INTERFACE - header-only material vocabulary)
# ============================================================================
file(GLOB TgeMaterial_HEADERS CONFIGURE_DEPENDS
	code/public/tge/material/*.hpp
)

add_library(TgeMaterial INTERFACE)

target_sources(TgeMaterial INTERFACE ${TgeMaterial_HEADERS})

target_link_libraries(TgeMaterial
	INTERFACE TgeMath
)

# ============================================================================
# TgeCore (INTERFACE - convenience all-in-one target)
# ============================================================================
add_library(TgeCore INTERFACE)

target_link_libraries(TgeCore INTERFACE
	TgeBase
	TgeLogging
	TgeMemory
	TgeMath
	TgeThreading
	TgeIO
	TgeInit
	TgeProfiling
	TgeModule
	TgeGeometry
	TgeMaterial
)

# TgeLifecycle is deliberately absent: it drives modules, and only a composition root does that.
