cmake_minimum_required(VERSION 3.16)
project(bachelier LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bachelier_core
    src/black_scholes.cpp
    src/implied_vol.cpp
    src/binomial.cpp
)
target_include_directories(bachelier_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(bachelier_core PRIVATE /W4)
else()
    target_compile_options(bachelier_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(bachelier src/main.cpp)
target_link_libraries(bachelier PRIVATE bachelier_core)

enable_testing()
add_executable(bachelier_tests tests/tests.cpp)
target_link_libraries(bachelier_tests PRIVATE bachelier_core)
add_test(NAME bachelier_tests COMMAND bachelier_tests)
