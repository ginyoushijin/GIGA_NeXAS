@echo off
cmake -A win32 -S . -B build
cmake --build build --config Release
copy /y .\quote\dll\*.dll .\build\Release
copy /y Readme.md .\build\Release