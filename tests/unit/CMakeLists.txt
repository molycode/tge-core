# Pure-logic units: decided from plain values alone, so they need no engine bring-up. That is what lets core
# prove them with nothing above core in the build, and it is why the threading suite is a separate binary --
# CJobGroup and ParallelFor need the scheduler running.

if(NOT TARGET GTest::gtest)
	find_package(GTest 1.14 REQUIRED)
	message(STATUS "[TgeCore] GTest (unit tests): package ${GTest_VERSION} from ${GTest_DIR}")
else()
	message(STATUS "[TgeCore] GTest (unit tests): already provided")
endif()

add_executable(TgeCoreUnitTests
	main.cpp
	test_event_system.cpp
	test_exposure_curve.cpp
	test_normal_matrix.cpp
	test_transform_decompose.cpp
	test_command_args.cpp
	test_command_registry.cpp
	test_module_graph.cpp
)

# The listener test drives the log system itself, so a build without one has nothing for it to assert on.
if(TGE_ENABLE_LOGGING)
	target_sources(TgeCoreUnitTests PRIVATE test_log_listener.cpp)
endif()

target_link_libraries(TgeCoreUnitTests
	PRIVATE
		GTest::gtest
		Tge::Events
		Tge::Exposure
		Tge::Math
		Tge::Command
		Tge::Logging
		Tge::Lifecycle
)

# The dispatcher and the module graph are private by design; reaching in beats widening a public header.
target_include_directories(TgeCoreUnitTests
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../../code/private/events
		${CMAKE_CURRENT_SOURCE_DIR}/../../code/private/module
)

add_test(NAME TgeCoreUnitTests COMMAND TgeCoreUnitTests)

set_target_properties(TgeCoreUnitTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
