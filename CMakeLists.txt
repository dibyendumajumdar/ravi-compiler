cmake_minimum_required(VERSION 3.12)
project(RaviCompiler VERSION 0.0.1 LANGUAGES C)

option(LUA_BINDING "Controls whether Lua/Ravi bindings will be generated, default is OFF" OFF)

set(HEADERS
        include/ravi_compiler.h
        src/allocate.h
        src/port.h
        src/ptrlist.h
        src/fnv_hash.h
        src/hash_table.h
        src/set.h
        src/membuf.h
        src/lua_defs.h
        src/ravi_ast.h)

set(SRCS
        src/allocate.c
        src/ast_walker.c
        src/ptrlist.c
        src/fnv_hash.c
        src/hash_table.c
        src/set.c
        src/lexer.c
        src/parser.c
        src/print.c
        src/typechecker.c
        src/linearizer.c
        src/membuf.c
        )

message("SOURCE dir is ${RaviCompiler_SOURCE_DIR}")

if ($ENV{CLION_IDE})
    # CLion seems unable to handle include paths set on sources
    include_directories("${RaviCompiler_SOURCE_DIR}/dmr_c/src")
endif ()

if (WIN32)
    # disable warnings about C string functions
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
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

add_executable(test_stringset tests/tstrset.c)
target_link_libraries(test_stringset ravicomp)
target_include_directories(test_stringset
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
        PUBLIC "${RaviCompiler_SOURCE_DIR}/include")

add_executable(tparse tests/tparse.c)
target_link_libraries(tparse ravicomp)
target_include_directories(tparse
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
        PUBLIC "${RaviCompiler_SOURCE_DIR}/include")

