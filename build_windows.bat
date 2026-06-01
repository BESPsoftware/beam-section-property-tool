@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
set "PROJECT_DIR=%~dp0"
set "BUILD_DRIVE="
for %%D in (S T U V W X Y Z) do (
    if not exist %%D:\ (
        set "BUILD_DRIVE=%%D:"
        goto :FoundDrive
    )
)
echo No available drive letters found.
exit /b 1

:FoundDrive
subst !BUILD_DRIVE! "!PROJECT_DIR!"
if errorlevel 1 (
    echo Failed to map drive !BUILD_DRIVE! to "!PROJECT_DIR!"
    exit /b 1
)
set "BUILD_DIR=!BUILD_DRIVE!\bsp_build"

if exist "%BUILD_DIR%" (
    echo Removing stale build directory %BUILD_DIR%
    rd /s /q "%BUILD_DIR%"
)

echo Configuring project in %BUILD_DIR%
cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo Configure failed
    subst !BUILD_DRIVE! /d >nul 2>&1
    exit /b 1
)

echo Building Release configuration
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo Build failed
    subst !BUILD_DRIVE! /d >nul 2>&1
    exit /b 1
)

echo Running tests
ctest --test-dir "%BUILD_DIR%" -C Release --output-on-failure
if errorlevel 1 (
    echo Tests failed
    subst !BUILD_DRIVE! /d >nul 2>&1
    exit /b 1
)

echo Windows build completed successfully.
subst !BUILD_DRIVE! /d >nul 2>&1
endlocal
