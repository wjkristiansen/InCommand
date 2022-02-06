add_library(InCommandLib STATIC
    InCommand.cpp
)

if(MSVC)
    target_compile_options(InCommandLib PRIVATE /W4 /WX)
else()
    target_compile_options(InCommandLib PRIVATE -Wall -Wextra -Werror)
endif()

