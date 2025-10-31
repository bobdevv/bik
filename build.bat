@echo off
echo Building Bik...

if not exist build mkdir build
cd build

echo Running CMake...
cmake .. -DBUILD_GUI=ON

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo Building project...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo Executables are in: build\Release\
echo   - bik.exe (CLI)
echo   - bik_gui.exe (GUI, if FLTK was found)
echo.
pause
