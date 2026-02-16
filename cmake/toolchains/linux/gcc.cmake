set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT DEFINED CMAKE_C_COMPILER)
	if(DEFINED ENV{TGE_GCC_PATH})
		set(CMAKE_C_COMPILER "$ENV{TGE_GCC_PATH}/bin/gcc")
	else()
		set(CMAKE_C_COMPILER "gcc")
	endif()
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
	if(DEFINED ENV{TGE_GCC_PATH})
		set(CMAKE_CXX_COMPILER "$ENV{TGE_GCC_PATH}/bin/g++")
	else()
		set(CMAKE_CXX_COMPILER "g++")
	endif()
endif()

add_compile_definitions(TGE_PLATFORM_LINUX)

if(DEFINED ENV{TGE_GCC_PATH})
	message(STATUS "Using GCC from TGE_GCC_PATH: $ENV{TGE_GCC_PATH}")
else()
	message(STATUS "Using system GCC (set TGE_GCC_PATH to use custom GCC)")
endif()

message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
