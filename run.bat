@echo off
set "BUILD_DIR=build"

echo Ensuring build directory exists...
if not exist "%BUILD_DIR%" (
mkdir "%BUILD_DIR%"
echo Directory "%BUILD_DIR%" created.
) else (
echo Directory "%BUILD_DIR%" already exists. Proceeding with CMake.
)

echo Starting CMake configuration...
cd "%BUILD_DIR%"
rem The '..' tells CMake to look for CMakeLists.txt one directory up
cmake ..

if %errorlevel% neq 0 (
echo.
echo ERROR: CMake configuration failed.
echo.
) else (
echo.
echo SUCCESS: CMake project files generated in the "%BUILD_DIR%" folder.
echo You can now open the Visual Studio solution file (owala.sln) inside the "%BUILD_DIR%" folder.
echo.
)

cd ..
pause