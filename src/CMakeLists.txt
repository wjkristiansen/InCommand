add_subdirectory(lib)

if( IN_COMMAND_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

if( IN_COMMAND_SAMPLE)
    add_subdirectory(sample)
endif()