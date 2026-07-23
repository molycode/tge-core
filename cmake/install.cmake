include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tge_module_package.cmake)

TgeVerifyDeclaredVersion(${CMAKE_CURRENT_SOURCE_DIR}/code/private/module/module_id.cpp ${PROJECT_VERSION})

# Recorded in the package config so a consumer can see which contract the installed binaries speak.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/code/public/tge/module/module.hpp moduleContractSource)

if(NOT moduleContractSource MATCHES "ModuleContractVersion\\{ *([0-9]+) *\\}")
	message(FATAL_ERROR "tge-core: no ModuleContractVersion literal found in module.hpp")
endif()

set(TGE_MODULE_CONTRACT_VERSION ${CMAKE_MATCH_1})

set(tgeCoreTargets
	TgeBase
	TgeLogging
	TgeMemory
	TgeMath
	TgeThreading
	TgeIO
	TgeInit
	TgeProfiling
	TgeModule
	TgeLifecycle
	TgeGeometry
	TgeMaterial
	TgeCore
)

TgeSetExportNames(${tgeCoreTargets})

install(TARGETS
		TgeBase
		TgeLogging
		TgeMemory
		TgeMath
		TgeThreading
		TgeIO
		TgeInit
		TgeProfiling
		TgeModule
		TgeLifecycle
		TgeGeometry
		TgeMaterial
		TgeCore
		rpmalloc
	EXPORT TgeCoreTargets
	ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY code/public/tge
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.hpp"
)

# glm reaches consumers through tge/math/glm_include.hpp, so it must travel with the package.
install(DIRECTORY external/glm/glm
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.hpp" PATTERN "*.inl" PATTERN "*.h"
)

install(EXPORT TgeCoreTargets
	FILE      TgeCoreTargets.cmake
	NAMESPACE Tge::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TgeCore
)

configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/TgeCoreConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/TgeCoreConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TgeCore
)

# SameMajorVersion mirrors Satisfies() in tge/module/version.hpp, so the build-time and load-time
# constraints cannot drift into disagreeing about what "compatible" means.
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/TgeCoreConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/TgeCoreConfig.cmake
		${CMAKE_CURRENT_BINARY_DIR}/TgeCoreConfigVersion.cmake
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/tge_module_package.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TgeCore
)
