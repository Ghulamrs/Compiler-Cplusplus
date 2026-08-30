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
        # A case that runs must also RUN, and must print the RIGHT thing: a
        # compiler can emit the wrong opcode, exit 0, and print nonsense.
        # Checking only the exit status let exactly that through on MSVC, so
        # the byte-exact comparison from tests/run_exec.sh is expressed in
        # CMake here and runs on every platform, not only where a POSIX shell
        # exists.  run_exec.sh remains the place to RECORD an expectation
        # (--accept); this is the place that enforces it.
        # A `trap_` case is one whose POINT is that the VM stops the program:
        # it must exit 3 with the right message.  Without a category for it a
        # guard can only ever look like a failure, so the guards -- a null
        # dereference, a double delete -- went untested.  run_exec.sh selects
        # on *run_*, so these are invisible to it and it needs no change.
        set(want_status 0)
        if(case_name MATCHES "trap_")
            set(want_status 3)
        endif()
        if(case_name MATCHES "run_|trap_")
            add_test(NAME ${case_name}_exec
                     COMMAND ${CMAKE_COMMAND}
                             -DEXE=$<TARGET_FILE:compilerpp>
                             -DSRC=${case_path}
                             -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected_run/${case_name}.txt
                             -DWANT_STATUS=${want_status}
                             -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_run.cmake)
            # A defect in the VM can outlast its own step limit -- a corrupted
            # free list spins inside allocate(), where nothing counts steps.
            set_tests_properties(${case_name}_exec PROPERTIES TIMEOUT 60)
        endif()
    endif()
endforeach()
