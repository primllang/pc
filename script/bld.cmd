@echo off 
set config=%1
if "%config%"=="" set config=Debug
cmake --build ./build --config %config%
 
