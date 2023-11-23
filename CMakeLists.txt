cmake_minimum_required(VERSION 3.15)

project(InCommand
    VERSION 1.0.0.3
    DESCRIPTION "InCommand command line library"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(IN_COMMAND_TEST "Enable gtest-based tests" OFF)
option(IN_COMMAND_SAMPLE "Enable sample app" OFF)

include_directories(
    inc
)

add_subdirectory(src)

if( IN_COMMAND_TEST )
    cmake_policy(SET CMP0135 NEW)
    
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    include(CTest)
    add_test(
        NAME InCommandTest COMMAND InCommandTest
    )
endif()