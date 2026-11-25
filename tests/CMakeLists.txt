# Core's own proof. Nothing here may name a module: core sits below every one of them, so a test that needs
# a renderer, a scene or a window belongs to whoever composes those.

# Under Node a web test ends as a native one does, static destructors included, and a threaded one starts with its
# workers there, as a page must.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	add_link_options(-sEXIT_RUNTIME=1)

	if(TGE_WEB_THREADS)
		add_link_options(-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency)
	endif()
endif()

add_subdirectory(unit)
add_subdirectory(zero_workers)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	add_subdirectory(frame_loop)
	add_subdirectory(user_folders)
endif()

# A build without threads has no workers for these to measure.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR TGE_WEB_THREADS)
	add_subdirectory(threading)
endif()

if(TARGET TgeIO)
	add_subdirectory(io)
endif()

if(TARGET TgeProcess)
	add_subdirectory(process)
endif()

# The global allocator option decides whether rpmalloc reaches the link; read that off a binary that links the heap.
if(CMAKE_NM AND TARGET TgeCoreThreadingTests AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	add_test(NAME TgeCoreHeapBackend
		COMMAND ${CMAKE_COMMAND}
			-DNM=${CMAKE_NM}
			-DBINARY=$<TARGET_FILE:TgeCoreThreadingTests>
			-DEXPECT_RPMALLOC=${TGE_ENABLE_GLOBAL_ALLOCATOR}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/check_heap_backend.cmake
	)
endif()
