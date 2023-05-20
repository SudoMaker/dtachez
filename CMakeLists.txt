cmake_minimum_required(VERSION 3.0)
project(dtachez C CXX)

set(CMAKE_CXX_STANDARD 11)

add_compile_options(-g -Os)

if (CMAKE_C_COMPILER MATCHES "mips")
    message("-- Optimizing for Ingenic MIPS platform")
    set(CFLAGS_COMMON -march=mips32r2 -mno-interlink-compressed -mno-shared -Os
            --param=l1-cache-size=16 --param=l1-cache-line-size=32 --param=l2-cache-size=128
            -ffunction-sections -fdata-sections
            -flto -ffat-lto-objects
            )
    add_compile_options(${CFLAGS_COMMON})
    add_link_options(${CFLAGS_COMMON} -Wl,-flto -Wl,--gc-sections)
endif()

add_executable(dtachez main.cpp attach.cpp master.cpp util.cpp)
target_link_libraries(dtachez util)
