cmake_minimum_required(VERSION 3.10)
project(problem_015)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler to g++-13 if available
find_program(GPP13 g++-13)
if(GPP13)
    set(CMAKE_CXX_COMPILER ${GPP13})
endif()

# Create executable named 'code' as required
add_executable(code main.cpp)

# Set optimization flags
target_compile_options(code PRIVATE -O2 -Wall -Wextra -Werror)

# For debugging, you can use -g flag
# target_compile_options(code PRIVATE -g)