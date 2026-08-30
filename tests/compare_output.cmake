# compare_output.cmake -- the comparison halves of run_tests.sh and
# run_exec.sh, in CMake script mode.
#
# Both shell suites do the same thing with different flags: run the compiler
# over a case and require its output to equal a recorded file.  Both need a
# POSIX shell, so on Windows neither ran, and the CMake tests that did run
# there checked only the exit status.  A compiler that emitted the wrong
# opcode still passed: the program ran, returned 0, and printed nonsense that
# nothing compared.  That is the exact failure run_exec.sh's own header warns
# about, and it hid a 26-case breakage on MSVC.
#
# Expressed here, the check runs wherever the build does.  It mirrors the
# shell versions rather than improving on them:
#
#   * stdout and stderr are MERGED, exactly as `2>&1` merges them there
#   * an unexpected exit status is a failure, reported apart from wrong output
#   * the merged output must equal the recorded file, byte for byte
#
# The shell scripts remain the place to RECORD an expectation (--accept, which
# is for reviewing a diff, never for blessing an unexamined result).  This is
# the place that enforces one.
#
#   MODE=dump   -ast -layout -ir -q   the syntax tree, layout and IR
#   MODE=run    -run -q               what the program prints
#
#   WANT_STATUS  0  a case that must be accepted, or must run
#                1  an err_ case, which must be rejected
#                3  a trap_ case, where the VM must stop the program
#
# Invoked by CTest as:
#   cmake -DEXE=<compilerpp> -DSRC=<case.cpp> -DEXPECT=<expected.txt>
#         -DMODE=<dump|run> [-DWANT_STATUS=n] -P compare_output.cmake

foreach(v EXE SRC EXPECT MODE)
    if(NOT DEFINED ${v})
        message(FATAL_ERROR "compare_output.cmake: -D${v} is required")
    endif()
endforeach()
if(NOT DEFINED WANT_STATUS)
    set(WANT_STATUS 0)
endif()

get_filename_component(case_name "${SRC}" NAME_WE)

if(MODE STREQUAL "dump")
    set(flags -ast -layout -ir -q)
elseif(MODE STREQUAL "run")
    set(flags -run -q)
else()
    message(FATAL_ERROR "compare_output.cmake: MODE must be 'dump' or 'run', got '${MODE}'")
endif()

if(NOT EXISTS "${EXPECT}")
    message(FATAL_ERROR
        "${case_name}: no expected file at ${EXPECT}.\n"
        "Record one with  tests/run_tests.sh <binary> --accept  (or run_exec.sh\n"
        "for a run_ case) and then CHECK it before committing.")
endif()

# CRLF vs LF, trailing spaces and trailing blank lines are artefacts of the
# platform and the editor, never of the compiler being tested.  Nothing else is
# normalised -- the point of these suites is that the output is exact.
function(normalise text out)
    string(REPLACE "\r\n" "\n" text "${text}")
    string(REPLACE "\r" "" text "${text}")
    string(REGEX REPLACE "[ \t]+\n" "\n" text "${text}")
    string(REGEX REPLACE "[ \t\n]+$" "" text "${text}")
    set(${out} "${text}" PARENT_SCOPE)
endfunction()

# One variable for both streams is how CMake merges them, matching `2>&1`.
# A case that reads cin gets input/NAME.txt; everything else reads nothing at
# all, so no test can sit waiting on a terminal.
get_filename_component(_case_dir "${SRC}" DIRECTORY)
set(_input "${_case_dir}/../input/${case_name}.txt")
if(EXISTS "${_input}")
    execute_process(
        COMMAND "${EXE}" ${flags} "${SRC}"
        INPUT_FILE "${_input}"
        OUTPUT_VARIABLE merged
        ERROR_VARIABLE merged
        RESULT_VARIABLE status
    )
else()
    execute_process(
        COMMAND "${EXE}" ${flags} "${SRC}"
        OUTPUT_VARIABLE merged
        ERROR_VARIABLE merged
        RESULT_VARIABLE status
    )
endif()

if(NOT status EQUAL WANT_STATUS)
    message("--- output ---\n${merged}")
    message(FATAL_ERROR "${case_name}: exited ${status}, expected ${WANT_STATUS}")
endif()

file(READ "${EXPECT}" expected)
normalise("${merged}" actual)
normalise("${expected}" expected)

if(NOT actual STREQUAL expected)
    message("--- expected ---\n${expected}")
    message("--- actual ---\n${actual}")
    message(FATAL_ERROR "${case_name}: output did not match ${EXPECT}")
endif()
