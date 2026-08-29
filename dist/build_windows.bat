@echo off
REM ---------------------------------------------------------------------------
REM  build_windows.bat -- build Compiler++ on Windows from the single-file
REM  amalgamation, with no project file and no CMake.
REM
REM  Put this next to compilerpp_amalgamated.cpp and double-click it, or run it
REM  from any command prompt:
REM
REM      build_windows.bat
REM
REM  It finds a compiler in this order: cl already on PATH, then Visual Studio
REM  via vswhere, then MinGW g++.  It builds, then runs a generated smoke test
REM  so you can see the compiler work without needing the tests folder.
REM ---------------------------------------------------------------------------
setlocal enabledelayedexpansion
cd /d "%~dp0"

if not exist compilerpp_amalgamated.cpp (
    echo ERROR: compilerpp_amalgamated.cpp is not in this folder.
    exit /b 2
)

REM --- 1. cl already on PATH? (a Developer Command Prompt) -------------------
where cl >nul 2>&1
if %ERRORLEVEL%==0 (
    echo Using MSVC already on PATH.
    goto :build_msvc
)

REM --- 2. find Visual Studio and load its environment ------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * ^
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
        -property installationPath`) do set "VSPATH=%%i"
)
if defined VSPATH (
    if exist "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" (
        echo Loading MSVC environment from !VSPATH!
        call "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" >nul
        goto :build_msvc
    )
)

REM --- 3. fall back to MinGW ------------------------------------------------
where g++ >nul 2>&1
if %ERRORLEVEL%==0 (
    echo Using MinGW g++.
    g++ -std=c++98 -Wall -Wextra -pedantic -o compilerpp.exe compilerpp_amalgamated.cpp
    if errorlevel 1 goto :failed
    goto :smoke
)

echo ERROR: no C++ compiler found.
echo   Install Visual Studio Build Tools, or put MinGW g++ on PATH.
exit /b 3

:build_msvc
echo Building with MSVC...
cl /nologo /W4 /EHsc /Fe:compilerpp.exe compilerpp_amalgamated.cpp
if errorlevel 1 goto :failed
goto :smoke

:failed
echo.
echo BUILD FAILED -- send the output above back and it will be fixed.
exit /b 1

:smoke
echo.
echo Build OK. Running a smoke test...
echo.
> smoke_test.cpp echo class Shape {
>> smoke_test.cpp echo public:
>> smoke_test.cpp echo   int side;
>> smoke_test.cpp echo   Shape^(int s^) : side^(s^) { }
>> smoke_test.cpp echo   virtual int area^(^) { return 0; }
>> smoke_test.cpp echo   virtual ~Shape^(^) { }
>> smoke_test.cpp echo };
>> smoke_test.cpp echo class Square : public Shape {
>> smoke_test.cpp echo public:
>> smoke_test.cpp echo   Square^(int s^) : Shape^(s^) { }
>> smoke_test.cpp echo   int area^(^) { return side * side; }
>> smoke_test.cpp echo   ~Square^(^) { }
>> smoke_test.cpp echo };
>> smoke_test.cpp echo int main^(^) {
>> smoke_test.cpp echo   Square sq^(4^);
>> smoke_test.cpp echo   Shape* s = ^&sq;
>> smoke_test.cpp echo   int ^&r = sq.side;
>> smoke_test.cpp echo   return s-^>area^(^) + r;
>> smoke_test.cpp echo }

compilerpp.exe -layout smoke_test.cpp
echo.
echo Exit code %ERRORLEVEL% (0 means the program was accepted).
endlocal
