# Core's own proof. Nothing here may name a module: core sits below every one of them, so a test that needs
# a renderer, a scene or a window belongs to whoever composes those.

add_subdirectory(unit)
add_subdirectory(threading)
add_subdirectory(zero_workers)
add_subdirectory(io)

if(TARGET TgeProcess)
	add_subdirectory(process)
endif()

# The global allocator option decides whether rpmalloc reaches the link; read that off a binary that links the heap.
if(CMAKE_NM)
	add_test(NAME TgeCoreHeapBackend
		COMMAND ${CMAKE_COMMAND}
			-DNM=${CMAKE_NM}
			-DBINARY=$<TARGET_FILE:TgeCoreThreadingTests>
			-DEXPECT_RPMALLOC=${TGE_ENABLE_GLOBAL_ALLOCATOR}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/check_heap_backend.cmake
	)
endif()
