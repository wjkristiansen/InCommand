add_executable(InCommandTest
    InCommandTest.cpp
)

# Link header interface target
target_link_libraries(InCommandTest PRIVATE
    InCommandLib
    InCommandHeaders
    gtest_main
)

# Discover individual test cases
include(GoogleTest)
gtest_discover_tests(InCommandTest)