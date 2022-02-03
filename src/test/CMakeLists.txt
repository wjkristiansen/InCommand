enable_testing()

add_executable(InCommandTest
    InCommandTest.cpp
)

target_link_libraries(InCommandTest
    InCommandLib
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(InCommandTest)