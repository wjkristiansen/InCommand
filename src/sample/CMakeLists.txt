add_executable(sample
    Sample.cpp
)


if(MSVC)
    target_compile_options(sample PRIVATE /W4 /WX)
else()
    target_compile_options(sample PRIVATE -Wall -Wextra -Werror)
endif()

# Link header interface target
target_link_libraries(sample PRIVATE
    InCommandLib
    InCommandHeaders)

if(MSVC)
    target_compile_options(sample PRIVATE /W4 /WX)
else()
    target_compile_options(sample PRIVATE -Wall -Wextra -Werror)
endif()

