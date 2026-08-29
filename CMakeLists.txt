# CMakeLists.txt -- builds Compiler++ anywhere, without Xcode.
#
# The compiler is plain C++98 using only the standard library, so the same
# build works with MSVC, MinGW, GCC and Clang.  Xcode remains the primary
# environment on macOS; this exists so Windows and Linux need no project file.
#
#   cmake -S . -B build
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# On Windows with Visual Studio:
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
#   cmake --build build --config Release

cmake_minimum_required(VERSION 3.10)
project(CompilerPlusPlus CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB COMPILERPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Compiler++/*.cpp")
add_executable(compilerpp ${COMPILERPP_SOURCES})

if(MSVC)
    # /W4 is MSVC's equivalent of -Wall -Wextra.
    target_compile_options(compilerpp PRIVATE /W4)
    # MSVC has no /std:c++98 switch; its default is close enough, and the
    # source uses nothing newer.  Nothing to set.
else()
    target_compile_options(compilerpp PRIVATE -Wall -Wextra -pedantic)
endif()

# --- tests -----------------------------------------------------------------
# Each case is one test: the compiler must accept it, or reject it when its
# name says "err_".  This is the exit-status half of tests/run_tests.sh, which
# needs a POSIX shell; expressed here it runs on Windows too.
enable_testing()
file(GLOB TEST_CASES "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/*.cpp")
foreach(case_path ${TEST_CASES})
    get_filename_component(case_name "${case_path}" NAME_WE)
    add_test(NAME ${case_name}
             COMMAND compilerpp -ast -layout -q "${case_path}")
    if(case_name MATCHES "err_")
        set_tests_properties(${case_name} PROPERTIES WILL_FAIL TRUE)
    else()
        # A case that runs must also RUN: a VM trap is a failure even though
        # the program compiled.  Byte-exact output is checked by
        # tests/run_exec.sh, which needs a POSIX shell; this half does not.
        if(case_name MATCHES "run_")
            add_test(NAME ${case_name}_exec
                     COMMAND compilerpp -run -q "${case_path}")
        endif()
    endif()
endforeach()
