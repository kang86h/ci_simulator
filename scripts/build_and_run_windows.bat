@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Move to repo root (one level up from scripts folder)
pushd "%~dp0.." || goto :fail

REM Ensure vcpkg is installed or clone it
if not exist "%CD%\vcpkg\vcpkg.exe" (
  echo [INFO] Cloning vcpkg...
  git clone https://github.com/microsoft/vcpkg.git || goto :fail
  call vcpkg\bootstrap-vcpkg.bat -disableMetrics || goto :fail
)

set VCPKG_DEFAULT_TRIPLET=x64-windows
set VCPKG_ROOT=%CD%\vcpkg
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

REM Install PortAudio
echo [INFO] Installing PortAudio via vcpkg...
"%VCPKG_ROOT%\vcpkg.exe" install portaudio --triplet %VCPKG_DEFAULT_TRIPLET% || goto :fail

REM Configure
if exist build\CMakeCache.txt (
  echo [INFO] Cleaning previous CMake cache...
  rmdir /s /q build
)
if not exist build mkdir build
echo [INFO] Configuring CMake...
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN_FILE% -DCMAKE_BUILD_TYPE=Release -DVCPKG_TARGET_TRIPLET=%VCPKG_DEFAULT_TRIPLET% || goto :fail

REM Build
echo [INFO] Building...
cmake --build build --config Release --parallel || goto :fail

REM Run (default params)
set EXE=build\Release\ci_rt_sim.exe
if not exist "%EXE%" set EXE=build\ci_rt_sim.exe
if not exist "%EXE%" (
  echo [ERROR] Executable not found: %EXE%
  goto :fail
)
echo [INFO] Running...
"%EXE%" --electrodes 22 --fmin 300 --fmax 7200 --lambda 0 --env-sr 250 --env-lpf 160 --sample-rate 48000 --block-frames 256
popd
goto :eof

:fail
echo [FAILED] See messages above. Fix issues and re-run.
popd
exit /b 1

endlocal


