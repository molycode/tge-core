# A program of its own, run again and again against one store: each run is a page load.
add_executable(TgeCoreUserFoldersTests main.cpp)

target_link_libraries(TgeCoreUserFoldersTests
	PRIVATE
		Tge::Core
		Tge::Lifecycle
)

target_link_options(TgeCoreUserFoldersTests PRIVATE --pre-js=${CMAKE_CURRENT_SOURCE_DIR}/indexed_db.js)

set_target_properties(TgeCoreUserFoldersTests PROPERTIES
	LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indexed_db.js
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

set(store ${CMAKE_CURRENT_BINARY_DIR}/store)

# Each follows the one before it, whose store it finds.
set(previous "")

function(AddStep name environment)
	add_test(NAME TgeCoreUserFolders${name} COMMAND ${ARGN})
	set_tests_properties(TgeCoreUserFolders${name} PROPERTIES
		ENVIRONMENT "${environment}"
		DEPENDS "${previous}"
		TIMEOUT 30
	)
	set(previous TgeCoreUserFolders${name} PARENT_SCOPE)
endfunction()

AddStep(Empty "" ${CMAKE_COMMAND} -E rm -f ${store})
AddStep(StoredAtTheEnd "TGE_TEST_STORE=${store}" TgeCoreUserFoldersTests --write one)
AddStep(ReadAfterTheEnd "TGE_TEST_STORE=${store}" TgeCoreUserFoldersTests --read one one)
AddStep(StoredByTheTick "TGE_TEST_STORE=${store}" TgeCoreUserFoldersTests --crash two)
AddStep(ReadAfterTheCrash "TGE_TEST_STORE=${store}" TgeCoreUserFoldersTests --read two one)

# These count what is said about it, which a build without logging does not say.
if(TGE_ENABLE_LOGGING)
	AddStep(StoreThatCannotLoad "TGE_TEST_STORE=${store};TGE_TEST_STORE_FAULT=get" TgeCoreUserFoldersTests --refused three)
	AddStep(StoreThatCannotStore "TGE_TEST_STORE=${store};TGE_TEST_STORE_FAULT=put" TgeCoreUserFoldersTests --refused three)
	AddStep(StoreThatCannotOpen "TGE_TEST_STORE=${store};TGE_TEST_STORE_FAULT=open" TgeCoreUserFoldersTests --refused three)
	AddStep(ReadAfterTheRefusals "TGE_TEST_STORE=${store}" TgeCoreUserFoldersTests --read two one)
	AddStep(NoStore "" TgeCoreUserFoldersTests --refused four)
endif()
