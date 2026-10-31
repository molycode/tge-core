add_executable(TgeCoreIOTests
	main.cpp
	test_is_busy.cpp
	test_read_whole_file.cpp
	test_support.cpp
	test_user_folder.cpp
	test_write_file_atomically.cpp
)

target_link_libraries(TgeCoreIOTests
	PRIVATE
		GTest::gtest
		Tge::IO
)

add_test(NAME TgeCoreIOTests COMMAND TgeCoreIOTests)

set_target_properties(TgeCoreIOTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
