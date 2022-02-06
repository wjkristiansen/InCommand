add_subdirectory(lib)

if( NOT DEFINED IN_COMMAND_NO_TEST)
    add_subdirectory(test)
endif()