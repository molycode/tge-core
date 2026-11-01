# A binary of its own, as Tge::Process stays outside Tge::Core; the child it starts is a program of its own too.

add_executable(TgeCoreProcessChild
	child_main.cpp
)

target_link_libraries(TgeCoreProcessChild
	PRIVATE
		Tge::Base
)

add_executable(TgeCoreProcessTests
	main.cpp
	test_child_process.cpp
	test_command_line.cpp
	test_get_program_path.cpp
	test_is_runnable.cpp
	test_startable_extension.cpp
	test_support.cpp
)

target_link_libraries(TgeCoreProcessTests
	PRIVATE
		GTest::gtest
		Tge::Process
)

# The command line and the startable extensions are private by design; reaching in beats widening a public header.
target_include_directories(TgeCoreProcessTests
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../private/process
)

target_compile_definitions(TgeCoreProcessTests
	PRIVATE
		TGE_PROCESS_CHILD="$<TARGET_FILE:TgeCoreProcessChild>"
		TGE_PROCESS_TESTS="$<TARGET_FILE:TgeCoreProcessTests>"
)

# Nothing else names the child, so building the tests has to build it.
add_dependencies(TgeCoreProcessTests TgeCoreProcessChild)

add_test(NAME TgeCoreProcessTests COMMAND TgeCoreProcessTests)

set_target_properties(TgeCoreProcessTests TgeCoreProcessChild PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
