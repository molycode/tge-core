# Common flags (C and C++)
add_compile_options(
	-g                      # Debug symbols
	-Wall                   # Enable most warnings
	-Wextra                 # Enable extra warnings
	-Werror                 # Treat warnings as errors
	-Wno-unused-parameter   # Suppress unused parameter warnings
)

# C++-only flags (add new C++ flags to this list)
add_compile_options(
	$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>    # Disable exceptions
)

message(STATUS "GCC ${CMAKE_CXX_COMPILER_VERSION} compiler flags configured")
