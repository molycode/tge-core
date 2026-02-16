cmake_minimum_required(VERSION 4.1.2)

project(tge-core LANGUAGES C CXX)

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

# Helper function to suppress warnings for external libraries (all supported compilers)
if(NOT COMMAND suppress_external_warnings)
function(suppress_external_warnings target_name)
	if(TARGET ${target_name})
		get_target_property(target_type ${target_name} TYPE)
		if(target_type STREQUAL "STATIC_LIBRARY" OR
		   target_type STREQUAL "SHARED_LIBRARY" OR
		   target_type STREQUAL "MODULE_LIBRARY" OR
		   target_type STREQUAL "OBJECT_LIBRARY" OR
		   target_type STREQUAL "EXECUTABLE")
			if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
				target_compile_options(${target_name} PRIVATE -w)
			elseif(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
				target_compile_options(${target_name} PRIVATE -w)
			elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
				target_compile_options(${target_name} PRIVATE /w)
			else()
				message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
			endif()
		endif()
	endif()
endfunction()
endif() # NOT COMMAND suppress_external_warnings

# Options
option(TGE_ENABLE_LOGGING "Enable logging system" ON)
option(TGE_ENABLE_MEMORY_TRACKING "Enable memory tracking system" ON)

if(PROJECT_IS_TOP_LEVEL)
	# Debug features in Debug builds
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_compile_definitions(TGE_DEBUG_ENABLED)
		message(STATUS "TGE Debug features ENABLED")
	else()
		message(STATUS "TGE Debug features DISABLED")
	endif()

	if(TGE_ENABLE_LOGGING)
		add_compile_definitions(TGE_LOGGING_ENABLED)
		message(STATUS "TGE Logging ENABLED")
	else()
		message(STATUS "TGE Logging DISABLED")
	endif()

	if(TGE_ENABLE_MEMORY_TRACKING)
		add_compile_definitions(TGE_MEMORY_TRACKING_ENABLED)
		message(STATUS "TGE Memory tracking ENABLED")
	else()
		message(STATUS "TGE Memory tracking DISABLED")
	endif()

	message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
endif() # PROJECT_IS_TOP_LEVEL

# External dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/rpmalloc.cmake)
suppress_external_warnings(rpmalloc)

# ============================================================================
# TgeBase (INTERFACE - header-only base utilities)
# ============================================================================
add_library(TgeBase INTERFACE)

target_include_directories(TgeBase INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/code/public
)

# ============================================================================
# TgeLogging (STATIC - logging system)
# ============================================================================
add_library(TgeLogging STATIC
	code/private/loggers.cpp
	code/private/logging/log_system.cpp
)

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
add_library(TgeMemory STATIC
	code/private/memory/default_allocator.cpp
	code/private/memory/linear_allocator.cpp
	code/private/memory/pool_allocator.cpp
	code/private/memory/stack_allocator.cpp
	code/private/memory/free_list_allocator.cpp
	code/private/memory/memory.cpp
	code/private/memory/global_operators.cpp
	code/private/memory/tracking.cpp
	code/private/memory/category_context.cpp
)

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
add_library(TgeMath STATIC
	code/private/math/transform.cpp
	code/private/math/intersection.cpp
)

target_link_libraries(TgeMath
	PUBLIC TgeBase
)

target_include_directories(TgeMath
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
)

# ============================================================================
# TgeThreading (STATIC - job system and thread pool)
# ============================================================================
add_library(TgeThreading STATIC
	code/private/threading/job_system.cpp
	code/private/threading/job_scheduler.cpp
	code/private/threading/thread_pool.cpp
	code/private/threading/job_group.cpp
)

target_link_libraries(TgeThreading
	PUBLIC TgeBase
	PRIVATE rpmalloc
)

target_include_directories(TgeThreading
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeIO (STATIC - file and path operations)
# ============================================================================
add_library(TgeIO STATIC
	code/private/io/file.cpp
	code/private/io/path.cpp
	code/private/io/directory.cpp
	code/private/io/stream.cpp
	code/private/io/io.cpp
)

target_link_libraries(TgeIO
	PUBLIC TgeBase
)

target_include_directories(TgeIO
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
)

# ============================================================================
# TgeInit (STATIC - initialization/termination)
# ============================================================================
add_library(TgeInit STATIC
	code/private/init.cpp
	code/private/assert.cpp
)

target_link_libraries(TgeInit
	PUBLIC TgeBase
	PRIVATE TgeMemory TgeThreading TgeIO TgeLogging
)

target_include_directories(TgeInit
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/code/private
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
)
