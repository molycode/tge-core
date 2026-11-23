add_executable(TgeCoreZeroWorkerTests
	main.cpp
	test_zero_workers.cpp
)

target_link_libraries(TgeCoreZeroWorkerTests
	PRIVATE
		GTest::gtest
		Tge::Core
)

target_include_directories(TgeCoreZeroWorkerTests
	PRIVATE ${PROJECT_SOURCE_DIR}/private
)

add_test(NAME TgeCoreZeroWorkerTests COMMAND TgeCoreZeroWorkerTests)

set_target_properties(TgeCoreZeroWorkerTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
