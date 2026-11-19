cmake_minimum_required(VERSION 3.10)
project(beam3_chess C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -O2)
option(ENABLE_SAN "sanitizers" OFF)
if(ENABLE_SAN)
  add_compile_options(-fsanitize=address,undefined -g)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(chess STATIC
  src/board.c
  src/movegen.c
  src/search.c
  src/eval.c
)
target_include_directories(chess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(chess_engine src/main.c)
target_link_libraries(chess_engine chess)
# popcnt support
target_compile_options(chess_engine PRIVATE -mpopcnt)
target_compile_options(chess PRIVATE -mpopcnt)

add_executable(tests tests/test_main.c)
target_link_libraries(tests chess)
target_compile_options(tests PRIVATE -mpopcnt)
add_test(NAME all_tests COMMAND tests)