set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT DEFINED CMAKE_C_COMPILER)
	if(DEFINED ENV{TGE_CLANG_PATH})
		set(CMAKE_C_COMPILER "$ENV{TGE_CLANG_PATH}/bin/clang")
	else()
		set(CMAKE_C_COMPILER "clang")
	endif()
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
	if(DEFINED ENV{TGE_CLANG_PATH})
		set(CMAKE_CXX_COMPILER "$ENV{TGE_CLANG_PATH}/bin/clang++")
	else()
		set(CMAKE_CXX_COMPILER "clang++")
	endif()
endif()

add_compile_definitions(TGE_PLATFORM_LINUX)

if(DEFINED ENV{TGE_CLANG_PATH})
	message(STATUS "Using Clang from TGE_CLANG_PATH: $ENV{TGE_CLANG_PATH}")
else()
	message(STATUS "Using system Clang (set TGE_CLANG_PATH to use custom Clang)")
endif()

message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
