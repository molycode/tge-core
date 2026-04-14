# add_external(source_dir [EXCLUDE_FROM_ALL] ...)
#
# Adds a vendored/submodule dependency as a subdirectory, isolated from the
# project's compiler flags. All targets produced by the subdirectory are
# automatically marked SYSTEM so their headers don't generate warnings in
# consuming code.
#
# Use this for all subdirectory-based external dependencies.
# For inline-defined targets (not via add_subdirectory), use suppress_external_warnings().
function(add_external source_dir)
	get_directory_property(_saved_opts COMPILE_OPTIONS)
	set_directory_properties(PROPERTIES COMPILE_OPTIONS "")

	add_subdirectory(${source_dir} ${ARGN})

	set_directory_properties(PROPERTIES COMPILE_OPTIONS "${_saved_opts}")

	get_property(_targets DIRECTORY ${source_dir} PROPERTY BUILDSYSTEM_TARGETS)
	foreach(_target ${_targets})
		if(TARGET ${_target})
			set_target_properties(${_target} PROPERTIES SYSTEM TRUE)
		endif()
	endforeach()
endfunction()

# suppress_external_warnings(target_name)
#
# Suppresses all compiler warnings for an inline-defined external target —
# one whose library is created directly in CMake (not via add_subdirectory).
# For subdirectory-based dependencies, prefer add_external() instead.
function(suppress_external_warnings target_name)
	if(TARGET ${target_name})
		get_target_property(_type ${target_name} TYPE)
		if(_type STREQUAL "STATIC_LIBRARY" OR
		   _type STREQUAL "SHARED_LIBRARY" OR
		   _type STREQUAL "MODULE_LIBRARY" OR
		   _type STREQUAL "OBJECT_LIBRARY" OR
		   _type STREQUAL "EXECUTABLE")
			if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
				target_compile_options(${target_name} PRIVATE /w)
			else()
				target_compile_options(${target_name} PRIVATE -w)
			endif()
		endif()
	endif()
endfunction()
