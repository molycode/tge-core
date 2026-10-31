add_executable(TgeCoreIOTests
	main.cpp
	test_user_folder.cpp
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
