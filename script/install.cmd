@echo off
setlocal
set DESTDIR=%1
cmake --build ./build -- install
endlocal
