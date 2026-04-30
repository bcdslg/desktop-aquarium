@echo off
setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "BUILD_DIR=%ROOT%\build"

where cmake >nul 2>nul
if errorlevel 1 (
    set "CMAKE="
    for %%V in (18 17) do (
        for %%E in (Community Professional Enterprise BuildTools) do (
            if exist "%ProgramFiles%\Microsoft Visual Studio\%%V\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
                set "CMAKE=%ProgramFiles%\Microsoft Visual Studio\%%V\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            )
            if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\%%V\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
                set "CMAKE=%ProgramFiles(x86)%\Microsoft Visual Studio\%%V\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            )
        )
    )
    if not defined CMAKE (
        echo CMake was not found on PATH.
        echo Install CMake or open a Visual Studio Developer Command Prompt and try again.
        exit /b 1
    )
) else (
    set "CMAKE=cmake"
)

echo === Configure ===
"%CMAKE%" -S "%ROOT%" -B "%BUILD_DIR%"
if errorlevel 1 exit /b 1

echo === Build Release ===
"%CMAKE%" --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

echo.
echo Built: %BUILD_DIR%\Release\DesktopAquarium.exe
