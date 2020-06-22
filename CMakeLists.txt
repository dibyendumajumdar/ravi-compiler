cmake_minimum_required(VERSION 3.12)
project(RaviCompiler VERSION 0.0.1 LANGUAGES C)

set(PUBLIC_HEADERS
    include/ravi_compiler.h)

set(HEADERS
        ${PUBLIC_HEADERS}
        src/allocate.h
        src/bitset.h
        src/ptrlist.h
        src/fnv_hash.h
        src/graph.h
        src/hash_table.h
        src/set.h
        src/membuf.h
        src/implementation.h)

set(SRCS
        src/allocate.c
        src/ast_walker.c
        src/bitset.c
        src/ptrlist.c
        src/fnv_hash.c
        src/graph.c
        src/cfg.c
        src/dominator.c
        src/hash_table.c
        src/set.c
        src/lexer.c
        src/parser.c
        src/ast_printer.c
        src/typechecker.c
        src/linearizer.c
        src/membuf.c
        )

message("SOURCE dir is ${RaviCompiler_SOURCE_DIR}")

if ($ENV{CLION_IDE})
    # CLion seems unable to handle include paths set on sources
    include_directories("${RaviCompiler_SOURCE_DIR}/include")
endif ()

if (WIN32)
    # disable warnings about C string functions
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wshadow -Wsign-compare -Wundef -Wwrite-strings \
        -Wredundant-decls -Wdisabled-optimization -Wdouble-promotion \
    	-Wlogical-op -Wno-aggressive-loop-optimizations \
        -Wmissing-prototypes -Wnested-externs \
        -Wstrict-prototypes -Werror=return-type -Wall -Wno-unused-parameter") # -Wc++-compat
endif()

include(GNUInstallDirs)

set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

add_library(ravicomp STATIC
        ${HEADERS}
        ${SRCS})
target_include_directories(ravicomp
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
        PUBLIC "${RaviCompiler_SOURCE_DIR}/include"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/src")
include(GenerateExportHeader)
generate_export_header(ravicomp)

add_executable(tmisc tests/tmisc.c)
target_link_libraries(tmisc ravicomp)
target_include_directories(tmisc
        PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/src"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/include")

add_executable(tparse tests/tparse.c tests/tcommon.c tests/tcommon.h)
target_link_libraries(tparse ravicomp)
target_include_directories(tparse
        PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/include")

add_executable(tastwalk tests/tastwalk.c tests/tcommon.c tests/tcommon.h)
target_link_libraries(tastwalk ravicomp)
target_include_directories(tastwalk
        PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/include")

add_executable(trun tests/trun.c tests/tcommon.c tests/tcommon.h)
target_link_libraries(trun ravicomp)
target_include_directories(trun
        PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/src"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/include")

add_executable(tgraph tests/tgraph.c)
target_link_libraries(tgraph ravicomp)
target_include_directories(tgraph
        PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/src"
        PRIVATE "${RaviCompiler_SOURCE_DIR}/include")
