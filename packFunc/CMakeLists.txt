aux_source_directory(. PACKFUNC_DIR)

add_library(PACKFUNC_LIB ${PACKFUNC_DIR})

target_include_directories(PACKFUNC_LIB PUBLIC ${CMAKE_SOURCE_DIR})