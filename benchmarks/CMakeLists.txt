# Micro-benchmarks. Kept out of the gtest suites on purpose: a timing assertion is a flaky test, but a tuning
# constant still has to come from a measurement.
#
# Core vendors no Google Benchmark for the same reason it vendors no googletest -- whoever supplies one turns
# TGE_CORE_BUILD_BENCHMARKS on.
if(NOT TARGET benchmark::benchmark)
	find_package(benchmark REQUIRED)
	message(STATUS "[TgeCore] Google Benchmark: package ${benchmark_VERSION} from ${benchmark_DIR}")
else()
	message(STATUS "[TgeCore] Google Benchmark: already provided")
endif()

add_executable(TgeCoreThreadingBenchmarks
	threading/bench_parallel_for.cpp
)

target_link_libraries(TgeCoreThreadingBenchmarks
	PRIVATE
		benchmark::benchmark
		Tge::Core
)

add_executable(TgeCoreEventsBenchmarks
	events/bench_event_system.cpp
)

target_link_libraries(TgeCoreEventsBenchmarks
	PRIVATE
		benchmark::benchmark
		Tge::Events
		Tge::EventsPublic
)

# CSystem is stack-constructible, so the benchmark drives the dispatcher directly -- no engine bring-up.
# Reaching into the private header beats widening a public one, the same call the unit suite makes.
target_include_directories(TgeCoreEventsBenchmarks
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../private/events
)

set_target_properties(TgeCoreThreadingBenchmarks TgeCoreEventsBenchmarks PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
)
