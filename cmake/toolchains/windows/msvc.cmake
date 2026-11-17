set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(NOMINMAX)
add_compile_options(/utf-8)

message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
