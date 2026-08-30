# compare_run.cmake -- the output half of run_exec.sh, in CMake script mode.
#
# run_exec.sh already does this job and does it well, but it needs a POSIX
# shell, so on Windows it does not run at all.  The CMake tests that DO run
# there checked only the exit status, which means a compiler that emits the
# wrong opcode still passed: the program ran, returned 0, and printed nonsense
# that nothing compared.  That is the exact failure run_exec.sh's own header
# warns about, and it hid a 26-case breakage on MSVC.
#
# This script is the same comparison expressed in the one language the build
# is guaranteed to have, so the check runs on every platform.  It deliberately
# mirrors run_exec.sh rather than improving on it:
#
#   * the compiler is invoked as  -run -q <case>
#   * stdout and stderr are MERGED, exactly as `2>&1` merges them there
#   * a non-zero exit is a failure, reported separately from a wrong answer
#   * the merged output must equal expected_run/<name>.txt
#
# Invoked by CTest as:
#   cmake -DEXE=<compilerpp> -DSRC=<case.cpp> -DEXPECT=<expected.txt>
#         -P compare_run.cmake

foreach(v EXE SRC EXPECT)
    if(NOT DEFINED ${v})
        message(FATAL_ERROR "compare_run.cmake: -D${v} is required")
    endif()
endforeach()

get_filename_component(case_name "${SRC}" NAME_WE)

if(NOT EXISTS "${EXPECT}")
    message(FATAL_ERROR
        "${case_name}: no expected_run file.\n"
        "Record one with  tests/run_exec.sh <binary> --accept  and then CHECK it.")
endif()

# CRLF vs LF, trailing spaces and trailing blank lines are artefacts of the
# platform and the editor, never of the compiler being tested.  Nothing else
# is normalised -- the point of this suite is that the answer is exact.
function(normalise text out)
    string(REPLACE "\r\n" "\n" text "${text}")
    string(REPLACE "\r" "" text "${text}")
    string(REGEX REPLACE "[ \t]+\n" "\n" text "${text}")
    string(REGEX REPLACE "[ \t\n]+$" "" text "${text}")
    set(${out} "${text}" PARENT_SCOPE)
endfunction()

# One variable for both streams is how CMake merges them, matching `2>&1`.
execute_process(
    COMMAND "${EXE}" -run -q "${SRC}"
    OUTPUT_VARIABLE merged
    ERROR_VARIABLE merged
    RESULT_VARIABLE status
)

if(NOT status EQUAL 0)
    message("--- output ---\n${merged}")
    message(FATAL_ERROR "${case_name}: exited ${status}, expected 0")
endif()

file(READ "${EXPECT}" expected)
normalise("${merged}" actual)
normalise("${expected}" expected)

if(NOT actual STREQUAL expected)
    message("--- expected ---\n${expected}")
    message("--- actual ---\n${actual}")
    message(FATAL_ERROR "${case_name}: wrong output")
endif()
