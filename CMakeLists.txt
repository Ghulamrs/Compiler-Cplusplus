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
# Both shell suites compare the compiler's output against a recorded file, and
# both need a POSIX shell, so on Windows neither ran.  What ran there checked
# only the exit status, which let a compiler that emitted the wrong opcode pass
# by running, returning 0, and printing nonsense that nothing compared.
#
# tests/compare_output.cmake is that comparison in CMake, so it runs wherever
# the build does.  Every case gets both halves the shell suites give it:
#
#   <name>        the syntax tree, layout and IR   (run_tests.sh, expected/)
#   <name>_exec   what the program prints          (run_exec.sh, expected_run/)
#
# The name says what a case must do -- err_ must be rejected, trap_ must be
# stopped by the VM, run_ must execute -- so the status each expects comes from
# the name rather than from a property.
enable_testing()
set(COMPARE "${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake")
file(GLOB TEST_CASES "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/*.cpp")
foreach(case_path ${TEST_CASES})
    get_filename_component(case_name "${case_path}" NAME_WE)

    # An err_ case is one the compiler must refuse, and refuse with the right
    # words: the dump file holds its diagnostics, so comparing it is what keeps
    # a reworded message a visible change rather than a silent one.
    set(want_status 0)
    if(case_name MATCHES "err_")
        set(want_status 1)
    endif()
    add_test(NAME ${case_name}
             COMMAND ${CMAKE_COMMAND}
                     -DEXE=$<TARGET_FILE:compilerpp>
                     -DSRC=${case_path}
                     -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${case_name}.txt
                     -DMODE=dump
                     -DWANT_STATUS=${want_status}
                     -P ${COMPARE})
    set_tests_properties(${case_name} PROPERTIES TIMEOUT 60)

    # A case that runs must also RUN, and must print the RIGHT thing.  A
    # `trap_` case is one whose POINT is that the VM stops the program: it must
    # exit 3 with the right message.  Without a category for it a guard could
    # only ever look like a failure, so the guards -- a null dereference, a
    # double delete -- went untested.  run_exec.sh selects on *run_*, so trap_
    # cases are invisible to it and it needs no change.
    if(case_name MATCHES "run_|trap_")
        set(want_status 0)
        if(case_name MATCHES "trap_")
            set(want_status 3)
        endif()
        add_test(NAME ${case_name}_exec
                 COMMAND ${CMAKE_COMMAND}
                         -DEXE=$<TARGET_FILE:compilerpp>
                         -DSRC=${case_path}
                         -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected_run/${case_name}.txt
                         -DMODE=run
                         -DWANT_STATUS=${want_status}
                         -P ${COMPARE})
        # A defect in the VM can outlast its own step limit -- a corrupted
        # free list spins inside allocate(), where nothing counts steps.
        set_tests_properties(${case_name}_exec PROPERTIES TIMEOUT 60)
    endif()
endforeach()

# --- malformed images ------------------------------------------------------
# Bytecode.h says a .cxb may have come from anywhere and that nothing in it is
# trusted.  These are hand-built files that load but are wrong in one specific
# way, and each must stop with a diagnostic rather than reach memory outside
# the VM's own array.  They run through the same comparator as a source case,
# because a .cxb is given to the compiler exactly as a .cpp is.
file(GLOB BAD_IMAGES "${CMAKE_CURRENT_SOURCE_DIR}/tests/images/*.cxb")
foreach(img ${BAD_IMAGES})
    get_filename_component(img_name "${img}" NAME_WE)
    add_test(NAME image_${img_name}
             COMMAND ${CMAKE_COMMAND}
                     -DEXE=$<TARGET_FILE:compilerpp>
                     -DSRC=${img}
                     -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/images/${img_name}.txt
                     -DWANT_STATUS=3
                     -DMODE=run
                     -P ${COMPARE})
    set_tests_properties(image_${img_name} PROPERTIES TIMEOUT 30)
endforeach()
