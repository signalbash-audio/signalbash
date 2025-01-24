@echo off
setlocal enabledelayedexpansion

:: Get number of logical processors for parallel builds
for /f "tokens=2 delims==" %%i in ('wmic cpu get NumberOfLogicalProcessors /value') do set NUM_CORES=%%i

echo Detected %NUM_CORES% logical processors for parallel building

:: Configure build
cmake -B build-windows ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release

:: Build all targets
echo Building VST3 plugin...
cmake --build build-windows --config Release --target Signalbash_VST3 -j%NUM_CORES%

echo Building CLAP plugin...
cmake --build build-windows --config Release --target Signalbash_CLAP -j%NUM_CORES%

echo Build complete!
