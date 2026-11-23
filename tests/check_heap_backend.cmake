execute_process(COMMAND ${NM} ${BINARY} OUTPUT_VARIABLE symbols RESULT_VARIABLE result)

if(NOT result EQUAL 0)
	message(FATAL_ERROR "nm could not read ${BINARY}")
endif()

string(FIND "${symbols}" "rpmalloc_initialize" position)

if(EXPECT_RPMALLOC AND position EQUAL -1)
	message(FATAL_ERROR "rpmalloc is missing from ${BINARY} although the global allocator is on")
elseif(NOT EXPECT_RPMALLOC AND NOT position EQUAL -1)
	message(FATAL_ERROR "rpmalloc reached ${BINARY} although the global allocator is off")
endif()
