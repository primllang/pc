@echo off
setlocal
set DESTDIR=%1
if "%DESTDIR%"=="" set DESTDIR=c:\x\pcdeploy
cmake --build ./build -- install
endlocal
