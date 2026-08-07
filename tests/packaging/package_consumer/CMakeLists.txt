# Resolves the module the way an outside project would: packages only, no path into any source tree.
cmake_minimum_required(VERSION 4.3.0)

project(TgeCorePackageConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CYGWIN)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()

find_package(TgeCore 1.0.0 REQUIRED)

# Requested without COMPONENTS, against a prefix that carries the harness: a Testing component that loaded
# because it is installed rather than because it was asked for would drag googletest into this configure and
# fail it. That is the shape of the bug this consumer exists to keep fixed.
if(TgeCore_Testing_FOUND)
	message(FATAL_ERROR "the Core package supplied its Testing component to a consumer that never asked")
endif()

add_executable(TgeCorePackageConsumer main.cpp)

# Lifecycle and Events by name, because Tge::Core carries neither. Composing a runtime and draining the event
# queue are the application's job, so the umbrella stops at what a module may link.
target_link_libraries(TgeCorePackageConsumer PRIVATE
	Tge::Core
	Tge::Lifecycle
	Tge::Events
)

if(NOT DEFINED TgeCore_MODULE_CONTRACT_VERSION)
	message(FATAL_ERROR "the installed TgeCoreConfig published no module contract version")
endif()

# Handed to the binary that will report what it actually compiled: the config's number is scraped out of
# module.hpp at install time, and a scrape that had drifted from the header would otherwise ship unnoticed.
target_compile_definitions(TgeCorePackageConsumer PRIVATE
	TGE_PACKAGE_CONTRACT_VERSION=${TgeCore_MODULE_CONTRACT_VERSION}
)
