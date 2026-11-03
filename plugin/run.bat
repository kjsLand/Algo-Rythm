@echo off
rem Configure and build the app from the root CMake project.
cd /d "%~dp0.."

cmake -B build -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b 1

cmake --build build --config Debug
if errorlevel 1 exit /b 1

rem To run the app: build\plugin\AlgoRythm_artefacts\Debug\Algo-Rythm.exe
