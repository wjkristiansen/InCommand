add_library(InCommandLib STATIC
    InCommand.cpp
)

if(MSVC)
    target_compile_options(InCommandLib PRIVATE /W4 /WX)
else()
    target_compile_options(InCommandLib PRIVATE -Wall -Wextra -Werror)
endif()

# Link header interface target
target_link_libraries(InCommandLib PRIVATE InCommandHeaders)

if(MSVC)
    target_compile_options(InCommandLib PRIVATE /W4 /WX)
else()
    target_compile_options(InCommandLib PRIVATE -Wall -Wextra -Werror)
endif()

