#!/bin/bash
#
# The module's own packaging proof, owning nothing of the engine tree but the one dependency it is told where
# to find. Proves five properties:
#   1. The module builds, tests and installs standalone, out of tree, against a googletest alone (steps 1-3).
#   2. Its build reaches outside the module for nothing (step 4).
#   3. A consumer resolving it from packages alone links the implementation, not just its identity (step 5),
#      and the package completes its own link interface (step 6).
#   4. The Testing component ships, and brings googletest with it (step 7).
#   5. That component is REQUESTED, never inherited: a consumer that never asked neither gets it nor needs
#      what it depends on (step 8), and a Core built without it says so (step 9).
#
# This is the module every other package's closure resolves through -- each sibling's install.cmake calls
# TgeVerifyDeclaredVersion out of this package -- so it is the worst one to leave unproven, and the last one
# to have acquired a gate.
#
# GOOGLETEST GETS A PREFIX OF ITS OWN, which the sibling gates have no reason to do. Every consuming step
# below runs against the Core prefix alone, so "no googletest on the search path" is a fact about where the
# consumer looks rather than about a prefix something deleted from -- and CMake's FindGTest module cannot
# quietly satisfy from the same prefix what config mode was supposed to. Steps 5 and 6 are therefore the
# standing regression for the COMPONENTS bug: Core installed WITH the harness, consumed by someone who never
# asked for it, on a path with no googletest at all.
#
# NO BENCHMARK HARNESS IS NEEDED ANYWHERE, and that is a decision rather than an omission: a benchmark is an
# instrument, not a gate. TGE_CORE_BUILD_BENCHMARKS stays off here.
#
# Usage: tests/packaging/verify_package_interface.sh [work-dir]

set -u

readonly MODULE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly WORK_DIR="${1:-${TMPDIR:-/tmp}/tge-core-package}"
readonly CMAKE="${TGE_CMAKE:-cmake}"

# Relative to the module, because the copy below has to carry its own: naming an absolute one would let the
# gate hand the copy a toolchain the module does not actually ship.
readonly TOOLCHAIN_REL="${TGE_TOOLCHAIN:-cmake/toolchains/linux/gcc.cmake}"

case "${TOOLCHAIN_REL}" in
	*clang*) readonly COMPILER_VAR="TGE_CLANG_PATH" ;;
	*)       readonly COMPILER_VAR="TGE_GCC_PATH"   ;;
esac

# Where googletest is checked out -- the module's only outside dependency, and only its harness wants one.
# Taken from the superproject when there is one; a relative guess would resolve against whatever directory
# happens to sit above a bare clone, which is a path that exists and answers the wrong question.
readonly DEPENDENCY_ROOT="${TGE_CORE_DEPENDENCY_ROOT:-$(git -C "${MODULE_ROOT}" rev-parse --show-superproject-working-tree 2>/dev/null)}"

readonly PREFIX="${WORK_DIR}/prefix"
readonly GTEST_PREFIX="${WORK_DIR}/prefix-gtest"
readonly PREFIX_NO_HARNESS="${WORK_DIR}/prefix-no-harness"

readonly MODULE_COPY="${WORK_DIR}/module-src"
readonly MODULE_BUILD="${WORK_DIR}/module"
readonly HARNESS_FREE_BUILD="${WORK_DIR}/module-no-harness"

failures=0

step()  { printf '\n=== %s\n' "$1"; }
pass()  { printf '  PASS  %s\n' "$1"; }
fail()  { printf '  FAIL  %s\n' "$1"; failures=$((failures + 1)); }
info()  { printf '  ....  %s\n' "$1"; }

configure()
{
	"${CMAKE}" -S "$1" -B "$2" -G Ninja \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_TOOLCHAIN_FILE="$3" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		"${@:4}" > "$2.configure.log" 2>&1
}

build_and_install()
{
	"${CMAKE}" --build "$1" > "$1.build.log" 2>&1 \
		&& "${CMAKE}" --install "$1" > "$1.install.log" 2>&1
}

readonly REQUIRED_CMAKE="$(sed -n 's/^cmake_minimum_required(VERSION \([0-9.]*\).*/\1/p' \
	"${MODULE_ROOT}/CMakeLists.txt" | head -1)"
readonly FOUND_CMAKE="$("${CMAKE}" --version 2>/dev/null | head -1 | awk '{print $3}')"

if [ -z "${FOUND_CMAKE}" ]; then
	printf 'FATAL: no usable cmake at "%s". Set TGE_CMAKE to one of at least %s.\n' \
		"${CMAKE}" "${REQUIRED_CMAKE}" >&2
	exit 1
fi

if [ "$(printf '%s\n%s\n' "${REQUIRED_CMAKE}" "${FOUND_CMAKE}" | sort -V | head -1)" != "${REQUIRED_CMAKE}" ]; then
	printf 'FATAL: "%s" is CMake %s, but this module requires %s. Set TGE_CMAKE to a newer one.\n' \
		"${CMAKE}" "${FOUND_CMAKE}" "${REQUIRED_CMAKE}" >&2
	exit 1
fi

if [ ! -f "${MODULE_ROOT}/${TOOLCHAIN_REL}" ]; then
	printf 'FATAL: no toolchain at "%s". Set TGE_TOOLCHAIN to a path under the module.\n' \
		"${MODULE_ROOT}/${TOOLCHAIN_REL}" >&2
	exit 1
fi

# Refusing beats measuring the wrong thing: unset, the toolchain silently falls back to whatever compiler
# came first on PATH, and every result below would describe a toolchain nobody asked for.
if [ -z "${!COMPILER_VAR:-}" ]; then
	printf 'FATAL: %s is unset, so "%s" would resolve the system compiler. Export it.\n' \
		"${COMPILER_VAR}" "${TOOLCHAIN_REL}" >&2
	exit 1
fi

if [ -z "${DEPENDENCY_ROOT}" ]; then
	printf 'FATAL: no dependency root, and this clone has no superproject to take one from. Set\n' >&2
	printf '       TGE_CORE_DEPENDENCY_ROOT to a checkout holding googletest.\n' >&2
	exit 1
fi

if [ ! -f "${DEPENDENCY_ROOT}/external/googletest/CMakeLists.txt" ]; then
	printf 'FATAL: no googletest under "%s". Set TGE_CORE_DEPENDENCY_ROOT.\n' "${DEPENDENCY_ROOT}" >&2
	exit 1
fi

# Named here rather than left to a configure failure two steps in: these are the module's own submodules, so
# an uninitialized one is a setup error and not a packaging result.
for submodule in external/glm/glm/glm.hpp external/rpmalloc/rpmalloc/rpmalloc.c
do
	if [ ! -f "${MODULE_ROOT}/${submodule}" ]; then
		printf 'FATAL: no "%s". Run: git submodule update --init --recursive\n' "${submodule}" >&2
		exit 1
	fi
done

# Step 4 asks whether the out-of-tree build names a path under this checkout, and a work dir inside it would
# make every path match. Refusing is the only answer that keeps that step meaning what it says.
case "${WORK_DIR}/" in
	"${MODULE_ROOT}/"*)
		printf 'FATAL: the work dir must sit outside the module, not at "%s".\n' "${WORK_DIR}" >&2
		exit 1
		;;
esac

readonly TOOLCHAIN_SRC="${MODULE_ROOT}/${TOOLCHAIN_REL}"

printf 'cmake      %s (%s)\ntoolchain  %s\n%-10s %s\ndeps       %s\n' \
	"${FOUND_CMAKE}" "${CMAKE}" "${TOOLCHAIN_REL}" \
	"${COMPILER_VAR}" "${!COMPILER_VAR}" "${DEPENDENCY_ROOT}"

rm -rf "${WORK_DIR}"
mkdir -p "${WORK_DIR}"

##############################################################################
step "1. Install googletest into a prefix of its own"

# Apart from Core's prefix on purpose -- see the header. This box has no system gtest, so a search path
# without this prefix on it is a path with no googletest of any kind.
if configure "${DEPENDENCY_ROOT}/external/googletest" "${WORK_DIR}/gtest" "${TOOLCHAIN_SRC}" \
		-DCMAKE_INSTALL_PREFIX="${GTEST_PREFIX}" -DINSTALL_GTEST=ON \
	&& build_and_install "${WORK_DIR}/gtest"
then
	pass "googletest installs to a prefix Core's does not share"
else
	fail "googletest install (see ${WORK_DIR}/gtest.*.log)"
fi

##############################################################################
step "2. The module builds and installs from OUTSIDE the engine tree"

mkdir -p "${MODULE_COPY}"

if tar -C "${MODULE_ROOT}" --exclude=./build --exclude=./.git -cf - . 2>/dev/null \
	| tar -C "${MODULE_COPY}" -xf - 2>/dev/null
then
	pass "module copied out of the engine tree ($(du -sh "${MODULE_COPY}" | cut -f1))"
else
	fail "could not copy the module to ${MODULE_COPY}"
fi

# The copy's OWN toolchain, not the engine's: that is the half an in-place build cannot measure, and the only
# thing that can catch the vendored cmake/ having been trimmed or left reaching at "../../".
#
# Harness and suite both on, which is the configuration a consuming tree that supplies googletest asks for --
# and the one that installs the second export set steps 7 to 9 are about.
if configure "${MODULE_COPY}" "${MODULE_BUILD}" "${MODULE_COPY}/${TOOLCHAIN_REL}" \
		-DCMAKE_PREFIX_PATH="${GTEST_PREFIX}" -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DTGE_CORE_TESTING=ON -DTGE_CORE_BUILD_TESTS=ON \
	&& build_and_install "${MODULE_BUILD}"
then
	pass "Core builds and installs out-of-tree, resolving googletest via find_package"
	# Ground truth rather than intent -- what the toolchain resolved to, so a run says what it measured.
	info "compiler $(sed -n 's/^set(CMAKE_CXX_COMPILER "\(.*\)")$/\1/p' \
		"${MODULE_BUILD}"/CMakeFiles/*/CMakeCXXCompiler.cmake | head -1)"
else
	fail "Core out-of-tree build (see ${MODULE_BUILD}.*.log)"
fi

# Written out by hand rather than listed from the prefix: the question is whether the install matches what a
# consumer is promised, and a list derived from the install can only ever agree with itself. One header per
# public namespace, so a directory the install pattern missed shows up as itself rather than as a compile
# error in somebody's module three weeks later.
missingArtifacts=""

for artifact in lib/cmake/TgeCore/TgeCoreConfig.cmake \
                lib/cmake/TgeCore/TgeCoreConfigVersion.cmake \
                lib/cmake/TgeCore/TgeCoreTargets.cmake \
                lib/cmake/TgeCore/TgeCoreTestingTargets.cmake \
                lib/cmake/TgeCore/tge_module_package.cmake \
                lib/libTgeLogging.a \
                lib/libTgeMemory.a \
                lib/libTgeMath.a \
                lib/libTgeThreading.a \
                lib/libTgeIO.a \
                lib/libTgeCommand.a \
                lib/libTgeInit.a \
                lib/libTgeModule.a \
                lib/libTgeLifecycle.a \
                lib/libTgeEventsPublic.a \
                lib/libTgeEvents.a \
                lib/librpmalloc.a \
                include/tge/config.hpp \
                include/tge/assert.hpp \
                include/tge/logging/log.hpp \
                include/tge/memory/allocator.hpp \
                include/tge/math/types.hpp \
                include/tge/threading/job_group.hpp \
                include/tge/io/file.hpp \
                include/tge/command/registry.hpp \
                include/tge/init/init.hpp \
                include/tge/module/module.hpp \
                include/tge/module/runtime.hpp \
                include/tge/events/system.hpp \
                include/tge/profiling/profiling.hpp \
                include/tge/geometry/vertex.hpp \
                include/tge/material/material_properties.hpp \
                include/tge/entity/entity.hpp \
                include/tge/light/light.hpp \
                include/tge/exposure/exposure_curve.hpp \
                include/tge/testing/expected_log_errors.hpp \
                include/glm/glm.hpp
do
	if [ ! -e "${PREFIX}/${artifact}" ]; then
		missingArtifacts="${missingArtifacts} ${artifact}"
	fi
done

if [ -z "${missingArtifacts}" ]; then
	pass "the install carries every archive, both export sets, the generated config header and glm"
else
	fail "the install is missing:${missingArtifacts}"
fi

##############################################################################
step "3. Both of the module's suites run against the out-of-tree build"

# Both, by name. A module added EXCLUDE_FROM_ALL has its test binaries excluded too, and a second suite that
# configures, builds nothing and reports nothing is indistinguishable from a passing gate -- so the loop
# names what it expects to find rather than running whatever happens to be there.
for suite in TgeCoreUnitTests TgeCoreThreadingTests
do
	binary="${MODULE_BUILD}/tests/${suite}"

	if [ -x "${binary}" ]; then
		if "${binary}" > "${WORK_DIR}/${suite}.run.log" 2>&1; then
			# Unfiltered, so any positive count is the real one; zero means the target registered nothing at
			# all, which a passing exit status would otherwise report as success.
			ranTests=$(sed -n 's/^\[==========\] \([0-9]*\) tests\? from .* ran\..*/\1/p' \
				"${WORK_DIR}/${suite}.run.log" | tail -1)

			if [ "${ranTests:-0}" -gt 0 ]; then
				pass "${ranTests} tests ran from ${suite}"
			else
				fail "${suite} registered no tests at all"
			fi
		else
			fail "${suite} failed (see ${WORK_DIR}/${suite}.run.log)"
			sed 's/^/        /' "${WORK_DIR}/${suite}.run.log" | tail -20
		fi
	else
		fail "no test binary at ${binary}"
	fi
done

##############################################################################
step "4. The out-of-tree build reaches outside the module for nothing"

# Two paths that should not appear, for two different reasons: the engine tree because the module vendors its
# own glm, rpmalloc and cmake/, and this checkout because the build under test is a COPY -- an absolute path
# baked into a cmake file would let the copy compile the original's sources and never say so.
if [ -f "${MODULE_BUILD}/compile_commands.json" ]; then
	reached=$(grep -o -e "${DEPENDENCY_ROOT}/[^\"[:space:]]*" -e "${MODULE_ROOT}/[^\"[:space:]]*" \
		"${MODULE_BUILD}/compile_commands.json" | sort -u)

	if [ -z "${reached}" ]; then
		pass "compile_commands.json names neither the engine tree nor this checkout"
	else
		fail "the out-of-tree build reaches outside the copy: ${reached}"
	fi
else
	fail "no compile_commands.json in ${MODULE_BUILD}"
fi

##############################################################################
step "5. A consumer resolving packages ALONE links the implementation"

# Core's prefix ALONE, and that is half the point: this Core was installed with its harness, so a Testing
# component that loaded because it is present rather than because it was asked for would drag a
# find_dependency(GTest) into this configure -- and there is no googletest on this path to satisfy it.
if configure "${MODULE_ROOT}/tests/packaging/package_consumer" "${WORK_DIR}/consumer" "${TOOLCHAIN_SRC}" \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
	&& "${CMAKE}" --build "${WORK_DIR}/consumer" > "${WORK_DIR}/consumer.build.log" 2>&1 \
	&& "${WORK_DIR}/consumer/TgeCorePackageConsumer" > "${WORK_DIR}/consumer.run.log" 2>&1
then
	# Exiting 0 only says the process ran; this line is the module's own archives having satisfied it.
	if grep -qF "linked the packaged Core implementation" "${WORK_DIR}/consumer.run.log"; then
		pass "consumer built from packages links the implementation and exits 0"
	else
		fail "consumer exited 0 but never linked the implementation (see ${WORK_DIR}/consumer.run.log)"
	fi
else
	fail "packaged consumer (see ${WORK_DIR}/consumer.*.log)"
fi

##############################################################################
step "6. The package completes its own link interface"

# Stricter than step 5 and that is the point: the consumer there names Lifecycle and Events itself, so it
# says nothing about what Tge::Core alone drags in. This probe names one target and links it, which is what
# forces rpmalloc -- reaching the link as a $<LINK_ONLY:> genex behind Tge::Memory -- to resolve out of the
# export set. It also asserts the packaging helpers every sibling's install.cmake calls.
if configure "${MODULE_ROOT}/tests/packaging/core_closure" "${WORK_DIR}/closure" "${TOOLCHAIN_SRC}" \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
	&& "${CMAKE}" --build "${WORK_DIR}/closure" > "${WORK_DIR}/closure.build.log" 2>&1 \
	&& "${WORK_DIR}/closure/TgeCoreClosure"
then
	pass "the closure probe resolves, links and runs from the package alone"
else
	fail "Core package closure (see ${WORK_DIR}/closure.*.log)"
fi

##############################################################################
step "7. The Testing component ships, and brings googletest with it"

# The probe resolves nothing for itself, so Tge::Testing's gtest edge can only arrive through the
# find_dependency the component declares. Built and run, not merely configured: generation evaluates the
# genex, only a link resolves the archive behind it, and only a run says the harness is viable.
if configure "${MODULE_ROOT}/tests/packaging/testing_closure" "${WORK_DIR}/testing" "${TOOLCHAIN_SRC}" \
		-DCMAKE_PREFIX_PATH="${PREFIX};${GTEST_PREFIX}" \
	&& "${CMAKE}" --build "${WORK_DIR}/testing" > "${WORK_DIR}/testing.build.log" 2>&1 \
	&& "${WORK_DIR}/testing/TgeCoreTestingClosure" > "${WORK_DIR}/testing.run.log" 2>&1
then
	pass "COMPONENTS Testing resolves the harness, its header and googletest"
else
	fail "Testing component closure (see ${WORK_DIR}/testing.*.log)"
fi

##############################################################################
step "8. NEGATIVE CONTROL: the Testing component's googletest is load-bearing"

# Same probe, same prefix, minus googletest's -- which here means simply not naming its prefix. If it still
# configures, the component is satisfying its harness from somewhere nobody declared, and step 7 proved less
# than it appears to.
if configure "${MODULE_ROOT}/tests/packaging/testing_closure" "${WORK_DIR}/testing-no-gtest" \
		"${TOOLCHAIN_SRC}" \
		-DCMAKE_PREFIX_PATH="${PREFIX}"
then
	fail "the Testing component configured with no googletest on the path -- find_dependency is not load-bearing"
else
	pass "asking for Testing without googletest fails the configure, as it must"
fi

##############################################################################
step "9. A Core built without the harness publishes the same package, bar the harness"

# No CMAKE_PREFIX_PATH at all: with the harness off there is nothing left to find, and a configure that still
# needed googletest would mean the option had stopped gating what it claims to.
if configure "${MODULE_COPY}" "${HARNESS_FREE_BUILD}" "${MODULE_COPY}/${TOOLCHAIN_REL}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX_NO_HARNESS}" \
	&& build_and_install "${HARNESS_FREE_BUILD}"
then
	pass "Core builds and installs with nothing whatever on its prefix path"
else
	fail "harness-free build (see ${HARNESS_FREE_BUILD}.*.log)"
fi

if [ -e "${PREFIX_NO_HARNESS}/lib/cmake/TgeCore/TgeCoreTestingTargets.cmake" ]; then
	fail "the harness-free install shipped a Testing export set anyway"
else
	pass "the harness-free install ships no Testing export set"
fi

# One Core version installing two packages that differ would make every claim above conditional on which
# build produced the prefix. The harness gets its own export set precisely so this comparison can be exact:
# every file a consumer reads must be byte-identical, and only TgeCoreTestingTargets* may be present at all.
differingFiles=""

for file in "${PREFIX}"/lib/cmake/TgeCore/*.cmake
do
	name="$(basename "${file}")"

	case "${name}" in
		TgeCoreTestingTargets*) ;;
		*)
			if ! cmp -s "${file}" "${PREFIX_NO_HARNESS}/lib/cmake/TgeCore/${name}"; then
				differingFiles="${differingFiles} ${name}"
			fi
			;;
	esac
done

if [ -z "${differingFiles}" ]; then
	pass "both installs publish byte-identical package config and export sets"
else
	fail "building the harness changed the package a consumer reads:${differingFiles}"
fi

if configure "${MODULE_ROOT}/tests/packaging/core_closure" "${WORK_DIR}/closure-no-harness" \
		"${TOOLCHAIN_SRC}" \
		-DCMAKE_PREFIX_PATH="${PREFIX_NO_HARNESS}" \
	&& "${CMAKE}" --build "${WORK_DIR}/closure-no-harness" > "${WORK_DIR}/closure-no-harness.build.log" 2>&1 \
	&& "${WORK_DIR}/closure-no-harness/TgeCoreClosure"
then
	pass "a Core consumer resolves, links and runs against the harness-free install"
else
	fail "harness-free closure (see ${WORK_DIR}/closure-no-harness.*.log)"
fi

# WITH googletest on the path, unlike step 8, and that inversion is the control: the only thing left that can
# fail this configure is the component itself being absent, so a check_required_components that had stopped
# rejecting an unavailable component would show up here and nowhere else.
if configure "${MODULE_ROOT}/tests/packaging/testing_closure" "${WORK_DIR}/testing-no-harness" \
		"${TOOLCHAIN_SRC}" \
		-DCMAKE_PREFIX_PATH="${PREFIX_NO_HARNESS};${GTEST_PREFIX}"
then
	fail "REQUIRED COMPONENTS Testing succeeded against a Core that never built the harness"
else
	pass "asking a harness-free Core for Testing fails the configure, as it must"
fi

##############################################################################
printf '\n'

if [ ${failures} -eq 0 ]
then
	printf 'ALL CHECKS PASSED (work dir: %s)\n' "${WORK_DIR}"
else
	printf '%d CHECK(S) FAILED (work dir: %s)\n' "${failures}" "${WORK_DIR}"
fi

exit $([ ${failures} -eq 0 ] && echo 0 || echo 1)
