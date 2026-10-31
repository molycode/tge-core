# A binary of its own: these need the socket library started, and Tge::Net stays outside Tge::Core.

add_executable(TgeCoreNetTests
	main.cpp
	test_address_lookup.cpp
	test_parse_ipv4.cpp
	test_stream_socket.cpp
	test_support.cpp
	test_udp_socket.cpp
	test_wait_for_readable.cpp
)

target_link_libraries(TgeCoreNetTests
	PRIVATE
		GTest::gtest
		Tge::Net
)

add_test(NAME TgeCoreNetTests COMMAND TgeCoreNetTests)

set_target_properties(TgeCoreNetTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
