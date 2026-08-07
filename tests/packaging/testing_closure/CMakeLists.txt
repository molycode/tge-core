# A closure probe for the Testing component: this project resolves nothing for itself, so the only thing that
# can satisfy Tge::Testing -- googletest above all -- is the find_dependency the component declares. It is
# also the arm that says the component is really optional: the same probe against a Core installed without
# the harness, or against a prefix with no googletest on it, must fail to configure.
cmake_minimum_required(VERSION 4.3.0)

project(TgeCoreTestingClosure LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CYGWIN)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()

find_package(TgeCore 1.0.0 REQUIRED COMPONENTS Testing)

if(NOT TgeCore_Testing_FOUND)
	message(FATAL_ERROR "the Core package accepted a REQUIRED Testing component it never provided")
endif()

# A real main.cpp, unlike the sibling probe's generated one: a bare main sees neither a header the install
# rule missed nor a googletest that arrived without its own link interface.
add_executable(TgeCoreTestingClosure main.cpp)

target_link_libraries(TgeCoreTestingClosure PRIVATE Tge::Testing)
