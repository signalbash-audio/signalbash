@echo off
setlocal enabledelayedexpansion

:: Detect logical processors
set "NUM_CORES=%NUMBER_OF_PROCESSORS%"
if not defined NUM_CORES (
    for /f %%i in ('powershell -NoProfile -Command "(Get-CimInstance -ClassName Win32_Processor).NumberOfLogicalProcessors"') do set "NUM_CORES=%%i"
)
echo Detected !NUM_CORES! logical processors for parallel building

:: Configure build
cmake -B build-windows ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release

:: Build all targets
echo Building VST3 plugin...
cmake --build build-windows --config Release --target Signalbash_VST3 -j!NUM_CORES!

echo Building CLAP plugin...
cmake --build build-windows --config Release --target Signalbash_CLAP -j!NUM_CORES!

echo Build complete!
