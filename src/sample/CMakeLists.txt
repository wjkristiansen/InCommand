add_executable(Sample
    Sample.cpp
)

target_link_libraries(Sample
    InCommandLib
    gtest_main
)

if(MSVC)
    target_compile_options(Sample PRIVATE /W4 /WX)
else()
    target_compile_options(Sample PRIVATE -Wall -Wextra -Werror)
endif()

