add_executable(TgeCoreZeroWorkerTests
	main.cpp
	test_zero_workers.cpp
)

target_link_libraries(TgeCoreZeroWorkerTests
	PRIVATE
		GTest::gtest
		Tge::Core
)

# Counting what a call logs needs the harness and a log system to count in.
if(TGE_ENABLE_LOGGING AND TARGET Tge::Testing)
	target_sources(TgeCoreZeroWorkerTests PRIVATE test_zero_worker_parallel_for.cpp)
	target_link_libraries(TgeCoreZeroWorkerTests PRIVATE Tge::Testing)
endif()

target_include_directories(TgeCoreZeroWorkerTests
	PRIVATE ${PROJECT_SOURCE_DIR}/private
)

add_test(NAME TgeCoreZeroWorkerTests COMMAND TgeCoreZeroWorkerTests)

set_target_properties(TgeCoreZeroWorkerTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
