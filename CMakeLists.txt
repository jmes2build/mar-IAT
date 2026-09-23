cmake_minimum_required(VERSION 3.16)
project(mariat LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mariat_core
    src/black_scholes.cpp
    src/implied_vol.cpp
    src/binomial.cpp
)
target_include_directories(mariat_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(mariat_core PRIVATE /W4)
else()
    target_compile_options(mariat_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(mariat src/main.cpp)
target_link_libraries(mariat PRIVATE mariat_core)

enable_testing()
add_executable(mariat_tests tests/tests.cpp)
target_link_libraries(mariat_tests PRIVATE mariat_core)
add_test(NAME mariat_tests COMMAND mariat_tests)
