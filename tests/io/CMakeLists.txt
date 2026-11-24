# A binary of its own, like the threading suite: the folder watch runs on an event loop, which needs Tge::Initialize.

add_executable(TgeCoreIOTests
	main.cpp
	test_directory_watch.cpp
	test_file.cpp
	test_is_busy.cpp
	test_read_whole_file.cpp
	test_support.cpp
	test_user_folder.cpp
	test_write_file_atomically.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(TgeCoreIOTests PRIVATE windows/test_read_whole_file.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_sources(TgeCoreIOTests PRIVATE linux/test_read_whole_file.cpp)
else()
	message(FATAL_ERROR "TgeCoreIOTests: no special file to read for ${CMAKE_SYSTEM_NAME}")
endif()

target_link_libraries(TgeCoreIOTests
	PRIVATE
		GTest::gtest
		Tge::Core
)

target_include_directories(TgeCoreIOTests
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME TgeCoreIOTests COMMAND TgeCoreIOTests)

set_target_properties(TgeCoreIOTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
