add_executable(chess_engine main.cpp board.cpp evaluate.cpp)

set_target_properties(chess_engine PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# Enable a consistent set of warnings across major compilers.
if(MSVC)
    target_compile_options(chess_engine PRIVATE /W4 /permissive- /Zc:preprocessor)
else()
    target_compile_options(chess_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
