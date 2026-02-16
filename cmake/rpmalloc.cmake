add_library(rpmalloc STATIC
	${CMAKE_CURRENT_LIST_DIR}/../external/rpmalloc/rpmalloc/rpmalloc.c
)

target_include_directories(rpmalloc
	PUBLIC
		${CMAKE_CURRENT_LIST_DIR}/../external/rpmalloc
)

target_compile_definitions(rpmalloc
	PRIVATE
		ENABLE_STATISTICS=0
		ENABLE_VALIDATE_ARGS=0
		ENABLE_OVERRIDE=0
)
