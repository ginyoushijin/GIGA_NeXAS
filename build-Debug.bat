@echo off
cmake -A win32 -S . -B build
cmake --build build
copy /y .\quote\dll\*.dll .\build\Debug
copy /y Readme.md .\build\Debug