# Runs CMD (a list) and fails if the exit code is not EXPECT.
# If OUTPUT_MATCH is set, the standard output and the standard error must contain it.
execute_process(
    COMMAND ${CMD}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
)
message("${out}${err}")
if (NOT result EQUAL EXPECT)
    message(FATAL_ERROR "Exit code ${result}, expected ${EXPECT}")
endif()
if (DEFINED OUTPUT_MATCH AND NOT "${out}${err}" MATCHES "${OUTPUT_MATCH}")
    message(FATAL_ERROR "The output does not contain '${OUTPUT_MATCH}'")
endif()
