cmake_minimum_required(VERSION 3.15)

project(InCommand
    VERSION 1.0
    DESCRIPTION "InCommand command line library"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(IN_COMMAND_TEST "Enable gtest-based tests" ON)

include_directories(
    inc
)

add_subdirectory(src)

if( IN_COMMAND_TEST )
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    include(CTest)
    add_test(
        NAME InCommandTest COMMAND InCommandTest.exe -arg1 arg2 --arg3 arg4:foo
    )
endif()