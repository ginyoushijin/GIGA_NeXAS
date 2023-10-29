cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)

project(GIGA_NEXAS LANGUAGES CXX)

add_subdirectory(huffman)

add_subdirectory(packFunc)

list(APPEND EXTRA_LIB HUFFMAN_LIB)

list(APPEND EXTRA_LIB PACKFUNC_LIB)

include_directories("quote/header/")

link_directories("quote/lib/")

list(APPEND EXTRA_DLL "zlib.dll" "libzstd.dll")

add_executable(GIGA_NeXAS main.cpp)

target_link_libraries(GIGA_NeXAS PUBLIC ${EXTRA_LIB} PUBLIC ${EXTRA_DLL})

add_custom_command(TARGET GIGA_NeXAS POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/Readme.md" $<TARGET_FILE_DIR:GIGA_NeXAS>)

file(GLOB EXTRA_DLL_FILES "quote/dll/*.dll")

foreach(singleDll ${EXTRA_DLL_FILES})
    add_custom_command(TARGET GIGA_NeXAS POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${singleDll} $<TARGET_FILE_DIR:GIGA_NeXAS>)
endforeach(singleDll)

