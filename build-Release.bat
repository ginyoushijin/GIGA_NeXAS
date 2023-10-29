@echo off
cmake -A win32 -S . -B build
cmake --build build --config Release