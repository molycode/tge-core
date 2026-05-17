add_library(rpmalloc STATIC
	${CMAKE_CURRENT_LIST_DIR}/../external/rpmalloc/rpmalloc/rpmalloc.c
)

target_include_directories(rpmalloc
	PUBLIC
		${CMAKE_CURRENT_LIST_DIR}/../external/rpmalloc
)

# On MSVC, C17 mode is required for _Static_assert and <stdatomic.h>.
# MSVC predefines __STDC_NO_ATOMICS__=1 even in C17 mode — it is a compiler-controlled
# macro that /U cannot clear.  We place a thin stdatomic.h shim before the system headers;
# it undefines the macro and then includes vcruntime_c11_stdatomic.h directly.
if(MSVC)
	target_compile_options(rpmalloc PRIVATE /std:c17)
	target_include_directories(rpmalloc BEFORE PRIVATE ${CMAKE_CURRENT_LIST_DIR}/msvc_compat)
endif()

target_compile_definitions(rpmalloc
	PRIVATE
		ENABLE_STATISTICS=0
		ENABLE_VALIDATE_ARGS=0
		ENABLE_OVERRIDE=0
)
