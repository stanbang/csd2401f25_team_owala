@echo off
set "BUILD_DIR=build"

echo Cleaning the project...
if exist "%BUILD_DIR%" (
rem Delete the build directory and all its contents (/S /Q)
rmdir /S /Q "%BUILD_DIR%"
echo Directory "%BUILD_DIR%" removed successfully.
) else (
echo Directory "%BUILD_DIR%" does not exist. Nothing to clean.
)
pause