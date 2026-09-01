@echo off
rem verify.cmd -- build Compiler++ with cl and run all six suites, on Windows.
rem
rem Why this file is in the repository rather than on the box: scaffold kept
rem only on the machine is scaffold that disappears when the machine is rebuilt,
rem and the failure then reads as a network fault rather than a missing script.
rem
rem   ssh windows "C:\Users\GRA\Documents\Compiler++\tests\windows\verify.cmd"
rem
rem Run it by its full path and with NO `cmd /c` in front: the default ssh shell
rem here is cmd, and that prefix nests cmd in cmd, strips the outer quotes, and
rem leaks one into %1.
rem
rem An optional first argument is the build directory; it defaults to C:\cppbuild.
rem Keep it OUT of the checkout -- a directory of .obj beside the sources is the
rem kind of thing a later wildcard build sweeps up.
setlocal enabledelayedexpansion

set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set "GITBASH=C:\Program Files\Git\bin\bash.exe"
set "ROOT=%~dp0..\.."
set "OUT=%~1"
if "%OUT%"=="" set "OUT=C:\cppbuild"

if not exist "%VCVARS%" echo verify: no vcvars64.bat at "%VCVARS%" & exit /b 1
if not exist "%GITBASH%" echo verify: no bash at "%GITBASH%" & exit /b 1

rem The redirect binds to the one command it follows, so `call ... >nul && set`
rem would leave call reading stdin. Parenthesised, and fed from NUL.
( call "%VCVARS%" >nul ) < NUL
if errorlevel 1 echo verify: vcvars64.bat failed & exit /b 1

if not exist "%OUT%" mkdir "%OUT%"
del /q "%OUT%\*.obj" 2>nul

echo === building with cl
pushd "%ROOT%"
cl -nologo -EHsc -W3 -Fo"%OUT%\\" -Fe:"%OUT%\compilerpp.exe" Compiler++\*.cpp
if errorlevel 1 (echo verify: build FAILED & popd & exit /b 1)
popd
echo === built "%OUT%\compilerpp.exe"

rem Everything below runs in Git bash, which is the only POSIX shell here. It is
rem `bash -c` and never `bash -lc`: a login shell rebuilds PATH from /etc/profile
rem and loses everything vcvars just added, so cl would be gone by the time
rem run_differential and run_amalgamated ask for it.
"%GITBASH%" -c "set -e; cd \"$(cygpath -u '%ROOT%')\"; BIN=\"$(cygpath -u '%OUT%')/compilerpp.exe\"; for s in run_tests run_exec run_roundtrip run_driver; do echo \"=== $s\"; sh tests/$s.sh \"$BIN\" | tail -2; done; echo '=== run_differential (cl)'; sh tests/run_differential.sh \"$BIN\" cl | tail -2; echo '=== run_amalgamated (cl)'; sh tests/run_amalgamated.sh \"$BIN\" cl | tail -2"
if errorlevel 1 (echo verify: a suite FAILED & exit /b 1)

echo === all six suites passed
exit /b 0
