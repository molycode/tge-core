# Packaging helpers shared by tge-core and the module repositories that find it.

# The package version duplicates the SVersion literal in module_id.cpp (a module bakes what it compiled
# against); checked here so the two cannot silently drift.
function(TgeVerifyDeclaredVersion moduleIdSourceFile expectedVersion)
	file(READ ${moduleIdSourceFile} moduleIdSource)

	if(NOT moduleIdSource MATCHES "SVersion\\{ *([0-9]+), *([0-9]+), *([0-9]+) *\\}")
		message(FATAL_ERROR "TgeVerifyDeclaredVersion: no SVersion literal in ${moduleIdSourceFile}")
	endif()

	set(declaredVersion "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")

	if(NOT declaredVersion VERSION_EQUAL expectedVersion)
		message(FATAL_ERROR
			"${moduleIdSourceFile} declares ${declaredVersion} but project() declares ${expectedVersion}")
	endif()
endfunction()

# install(EXPORT NAMESPACE) prefixes the real target name, not the alias, so without EXPORT_NAME a package
# hands out Tge::TgeEvents where the source path gives Tge::Events. A missing alias fails here, not silently.
function(TgeSetExportNames)
	foreach(target ${ARGN})
		string(REGEX REPLACE "^Tge" "" exportName "${target}")

		if(NOT TARGET Tge::${exportName})
			message(FATAL_ERROR "${target} is exported as Tge::${exportName}, but no such alias exists")
		endif()

		set_target_properties(${target} PROPERTIES EXPORT_NAME ${exportName})
	endforeach()
endfunction()
