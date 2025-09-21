@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Move to repo root
pushd "%~dp0.." || goto :fail

set TRIPLET=x64-windows-static
set VCPKG_ROOT=%CD%\vcpkg
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
  echo [INFO] Cloning vcpkg...
  git clone https://github.com/microsoft/vcpkg.git || goto :fail
  call vcpkg\bootstrap-vcpkg.bat -disableMetrics || goto :fail
)

echo [INFO] Installing portaudio (static)...
"%VCPKG_ROOT%\vcpkg.exe" install portaudio --triplet %TRIPLET% || goto :fail

if exist build-static\CMakeCache.txt rmdir /s /q build-static
mkdir build-static

echo [INFO] Configuring CMake (static)...
cmake -S . -B build-static -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%TRIPLET% -DCMAKE_BUILD_TYPE=Release || goto :fail

echo [INFO] Building (static)...
cmake --build build-static --config Release --parallel || goto :fail

set EXE=build-static\Release\ci_rt_sim.exe
if not exist "%EXE%" set EXE=build-static\ci_rt_sim.exe

if not exist dist mkdir dist
copy /Y "%EXE%" dist\ci_rt_sim_portable.exe >NUL
echo [SUCCESS] Portable EXE at dist\ci_rt_sim_portable.exe

popd
goto :eof

:fail
echo [FAILED] See messages above.
popd
exit /b 1


