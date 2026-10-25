@echo off
rem Tests loaded-game selection with standalone policy ON and normal Flycast policy OFF.
rem Usage: standalone_game_test.cmd [output directory] [optional private extracted chip directory]
setlocal
set ROOT=%~dp0..\..
set OUT=%~1
set FIXTURE=%~2
if "%OUT%"=="" set OUT=%ROOT%\build-win\vrtest\death-game-policy-unit
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT (
    echo Visual Studio with the C++ tools was not found.
    exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
if not exist "%OUT%" mkdir "%OUT%" || exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /W3 /I"%ROOT%\core" /DVR_STANDALONE_DEATHCRIMSONOX=1 /DEXPECT_STANDALONE_DEATHCRIMSONOX=1 /Festandalone-on.exe "%~dp0standalone_game_test.cpp" >build-on.log 2>&1
if errorlevel 1 goto build_failed
if defined FIXTURE (
    standalone-on.exe "%FIXTURE%"
) else (
    standalone-on.exe
)
if errorlevel 1 goto failed
cl /nologo /std:c++17 /EHsc /O2 /W3 /I"%ROOT%\core" /DEXPECT_STANDALONE_DEATHCRIMSONOX=0 /Festandalone-off.exe "%~dp0standalone_game_test.cpp" >build-off.log 2>&1
if errorlevel 1 goto build_failed
if defined FIXTURE (
    standalone-off.exe "%FIXTURE%"
) else (
    standalone-off.exe
)
if errorlevel 1 goto failed
cl /nologo /std:c++17 /EHsc /O2 /W3 /I"%ROOT%\core" /DVR_STANDALONE_DEATHCRIMSONOX=0 /DEXPECT_STANDALONE_DEATHCRIMSONOX=0 /Festandalone-zero.exe "%~dp0standalone_game_test.cpp" >build-zero.log 2>&1
if errorlevel 1 goto build_failed
if defined FIXTURE (
    standalone-zero.exe "%FIXTURE%"
) else (
    standalone-zero.exe
)
if errorlevel 1 goto failed
popd
exit /b 0
:build_failed
type build-on.log
if exist build-off.log type build-off.log
if exist build-zero.log type build-zero.log
:failed
popd
exit /b 1
