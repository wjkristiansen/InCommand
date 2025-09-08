enable_testing()

add_executable(InCommandTest
    InCommandTest.cpp
)


include(GoogleTest)
gtest_discover_tests(InCommandTest)

# Link header interface target
target_link_libraries(InCommandTest PRIVATE
    InCommandLib
    InCommandHeaders
    gtest_main)

include(GoogleTest)
gtest_discover_tests(InCommandTest)