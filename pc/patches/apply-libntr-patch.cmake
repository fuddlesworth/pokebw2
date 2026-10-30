set(patch "${CMAKE_CURRENT_LIST_DIR}/libntr-envelope.patch")
execute_process(
    COMMAND git -C "${PC_LIBNTR_SOURCE_DIR}" apply --reverse --check "${patch}"
    RESULT_VARIABLE already_applied
    OUTPUT_QUIET ERROR_QUIET)
if(NOT already_applied EQUAL 0)
    execute_process(
        COMMAND git -C "${PC_LIBNTR_SOURCE_DIR}" apply "${patch}"
        COMMAND_ERROR_IS_FATAL ANY)
endif()
