# A program of its own: the frames come once main has returned, so its exit status is the result.
add_executable(TgeCoreFrameLoopTests main.cpp)

target_link_libraries(TgeCoreFrameLoopTests PRIVATE Tge::Lifecycle)

add_test(NAME TgeCoreFrameLoopTests COMMAND TgeCoreFrameLoopTests)

add_test(NAME TgeCoreFrameLoopStatus COMMAND TgeCoreFrameLoopTests --refuse)
set_tests_properties(TgeCoreFrameLoopStatus PROPERTIES WILL_FAIL ON)

set_target_properties(TgeCoreFrameLoopTests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
