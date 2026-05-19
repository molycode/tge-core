add_compile_options(
	$<$<COMPILE_LANGUAGE:CXX>:/EHs- /EHc->  # Disable C++ exceptions
	$<$<COMPILE_LANGUAGE:CXX>:/wd4530>      # Unreferenced local exception handler (consequence of /EHs-)
	$<$<COMPILE_LANGUAGE:CXX>:/wd4100>      # Unreferenced formal parameter
)

message(STATUS "MSVC ${CMAKE_CXX_COMPILER_VERSION} compiler flags configured")
