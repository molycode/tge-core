# A binary of its own: these need the socket library started, and Tge::Net stays outside Tge::Core.

add_executable(TgeCoreNetTests
	main.cpp
	test_address_lookup.cpp
	test_certificates.cpp
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

# The certificate decoder is private by design; reaching in beats widening a public header.
target_include_directories(TgeCoreNetTests
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../private/net
)

add_test(NAME TgeCoreNetTests COMMAND TgeCoreNetTests)

set_target_properties(TgeCoreNetTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
