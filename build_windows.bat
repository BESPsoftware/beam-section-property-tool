@echo off
setlocal

set "QT_ROOT="
set "QT_VERSION="
set "BUILD_DIR="
set "GUI_EXE="

if exist "C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe" (
    set "QT_ROOT=C:\Qt\5.15.2\msvc2019_64"
    set "QT_VERSION=5"
    set "BUILD_DIR=build-qt-msvc"
    set "GUI_EXE=build-qt-msvc\Library\bin\Release\SectionPropertyGui.exe"
    goto :BuildMsvc
)

if exist "C:\Qt\6.11.1\mingw_64\bin\windeployqt.exe" (
    set "QT_ROOT=C:\Qt\6.11.1\mingw_64"
    set "QT_VERSION=6"
    set "BUILD_DIR=build-qt6-mingw"
    set "GUI_EXE=build-qt6-mingw\Library\bin\SectionPropertyGui.exe"
    goto :BuildMingw
)

echo No supported Qt installation found.
echo Expected either:
echo   C:\Qt\5.15.2\msvc2019_64
echo   C:\Qt\6.11.1\mingw_64
exit /b 1

:BuildMsvc
set "VS_GENERATOR=Visual Studio 17 2022"
if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2022" (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\18" (
        set "VS_GENERATOR=Visual Studio 18 2026"
    )
)

cmake -S . -B "%BUILD_DIR%" ^
    -G "%VS_GENERATOR%" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_ROOT%" ^
    -DSPT_QT_VERSION=%QT_VERSION%
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

set "QT_PLUGIN_PATH=%QT_ROOT%\plugins"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%QT_ROOT%\plugins\platforms"
ctest --test-dir "%BUILD_DIR%" -C Release --output-on-failure
if errorlevel 1 exit /b 1

"%QT_ROOT%\bin\windeployqt.exe" --release --no-translations "%GUI_EXE%"
if errorlevel 1 exit /b 1

goto :Done

:BuildMingw
if not exist "C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe" (
    echo Qt MinGW toolchain not found at C:\Qt\Tools\mingw1310_64\bin
    exit /b 1
)

set "PATH=C:\Qt\Tools\mingw1310_64\bin;%QT_ROOT%\bin;%PATH%"

cmake -S . -B "%BUILD_DIR%" ^
    -G "MinGW Makefiles" ^
    -DCMAKE_C_COMPILER="C:\Qt\Tools\mingw1310_64\bin\gcc.exe" ^
    -DCMAKE_CXX_COMPILER="C:\Qt\Tools\mingw1310_64\bin\g++.exe" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_ROOT%" ^
    -DSPT_QT_VERSION=%QT_VERSION%
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%"
if errorlevel 1 exit /b 1

set "PATH=%CD%\%BUILD_DIR%\Library\bin;%PATH%"
set "QT_PLUGIN_PATH=%QT_ROOT%\plugins"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%QT_ROOT%\plugins\platforms"
ctest --test-dir "%BUILD_DIR%" --output-on-failure
if errorlevel 1 exit /b 1

"%QT_ROOT%\bin\windeployqt.exe" --release --no-translations "%GUI_EXE%"
if errorlevel 1 exit /b 1

:Done
echo.
echo === Build and deployment complete ===
echo Launch: %GUI_EXE%
endlocal
