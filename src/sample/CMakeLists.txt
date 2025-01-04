add_executable(sample
Sample.cpp
)

target_link_libraries(sample
    InCommandLib
    gtest_main
)

if(MSVC)
    target_compile_options(sample PRIVATE /W4 /WX)
else()
    target_compile_options(sample PRIVATE -Wall -Wextra -Werror)
endif()

