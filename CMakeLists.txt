cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)

project(GIGA_NEXAS LANGUAGES CXX)

add_subdirectory(huffman)

add_subdirectory(packFunc)

list(APPEND EXTRA_LIB HUFFMAN_LIB)

list(APPEND EXTRA_LIB PACKFUNC_LIB)

include_directories("${PROJECT_SOURCE_DIR}/quote/header/")

link_directories("${PROJECT_SOURCE_DIR}/quote/lib/")

list(APPEND EXTRA_DLL "zlib.dll" "libzstd.dll")

add_executable(GIGA_NeXAS main.cpp)

target_link_libraries(GIGA_NeXAS PUBLIC ${EXTRA_LIB} PUBLIC ${EXTRA_DLL})

