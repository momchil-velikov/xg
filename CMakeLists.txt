project(xg)

cmake_minimum_required(VERSION 3.0)

add_subdirectory(ulib)

add_compile_options(-std=c11 -Wall -Wextra)

add_executable(xg conflicts.c first-follow.c gen-c-parser.c grammar.c
                  lalr.c lr0.c malloc.c parse.c random-gen.c symtab.c
                  xg.c)

target_include_directories(xg PUBLIC ${CMAKE_SOURCE_DIR}/ulib)
target_link_libraries(xg ulib)
