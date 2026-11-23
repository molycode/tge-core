# A second binary, and the split is the point: these need Tge::Initialize where every unit beside them needs nothing
# above core.

add_executable(TgeCoreThreadingTests
	main.cpp
	test_job_group.cpp
	test_parallel_for.cpp
)

# The web builds no event loop: nothing there waits on sockets.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	target_sources(TgeCoreThreadingTests PRIVATE test_event_loop.cpp)
endif()

target_link_libraries(TgeCoreThreadingTests
	PRIVATE
		GTest::gtest
		Tge::Core
)

add_test(NAME TgeCoreThreadingTests COMMAND TgeCoreThreadingTests)

set_target_properties(TgeCoreThreadingTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
