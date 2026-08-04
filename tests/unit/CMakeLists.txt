# Pure-logic units. The dispatcher is decided from plain values and threads alone, so it needs no engine
# bring-up -- driving CSystem directly is what lets core prove it with nothing above core in the build.

if(NOT TARGET GTest::gtest)
	find_package(GTest 1.14 REQUIRED)
endif()

add_executable(TgeCoreUnitTests
	main.cpp
	test_event_system.cpp
)

target_link_libraries(TgeCoreUnitTests
	PRIVATE
		GTest::gtest
		Tge::Events
)

# The dispatcher is private by design; reaching in beats widening a public header for a test.
target_include_directories(TgeCoreUnitTests
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../../code/private/events
)

add_test(NAME TgeCoreUnitTests COMMAND TgeCoreUnitTests)

set_target_properties(TgeCoreUnitTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
