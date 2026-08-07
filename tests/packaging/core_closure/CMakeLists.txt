# A closure probe: this project resolves nothing for itself, so the only thing that can satisfy Tge::Core's
# link interface is what the package itself carries. rpmalloc is the one that matters -- it reaches the link
# as a $<LINK_ONLY:> genex behind Tge::Memory, and nothing evaluates one of those until something is linked.
cmake_minimum_required(VERSION 4.3.0)

project(TgeCoreClosure LANGUAGES CXX)

find_package(TgeCore 1.0.0 REQUIRED)

# Every sibling module's install.cmake calls these and gets them from nowhere but this package. A missing
# file already fails the config's own include(); what this catches is the file still loading while a helper
# has been renamed out from under them -- which surfaces in each of THEM, at the install rule of a module
# that had already built.
foreach(helper TgeVerifyDeclaredVersion TgeSetExportNames)
	if(NOT COMMAND ${helper})
		message(FATAL_ERROR "the Core package defines no ${helper}, which every module's install rule calls")
	endif()
endforeach()

# It must be a real linked binary, not an INTERFACE library: the export set's own generated check covers only
# the targets it exports, and an archive it never opens is one it cannot find missing.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/closure.cpp "int main() { return 0; }\n")

add_executable(TgeCoreClosure ${CMAKE_CURRENT_BINARY_DIR}/closure.cpp)

target_link_libraries(TgeCoreClosure PRIVATE Tge::Core)
