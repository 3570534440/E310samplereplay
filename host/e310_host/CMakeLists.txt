cmake_minimum_required(VERSION 3.19)
set(CMAKE_SYSTEM_NAME Linux)
set (CMAKE_C_COMPILER "/media/wu/ubuntu20.04_D/work/software/vivado2019.1/vivdao2019/SDK/2019.1/gnu/aarch32/lin/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-gcc")
set (CMAKE_CXX_COMPILER "/media/wu/ubuntu20.04_D/work/software/vivado2019.1/vivdao2019/SDK/2019.1/gnu/aarch32/lin/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-g++")

set(CMAKE_SYSTEM_NAME Linux)

project(dma_test C)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)


set(CMAKE_C_FLAGS_DEBUG "-g") 
set(CMAKE_CXX_FLAGS_DEBUG "-g") 

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

aux_source_directory(./ SRC)
message(STATUS "SRC DIR ${SRC}")

set(INCLUDE "./")
##message(STATUS "INCLUDE: ${INCLUDE}")


add_executable(dma_test dma.c)

target_sources(dma_test PRIVATE ${SRC})
target_include_directories(dma_test PUBLIC ${INCLUDE})


target_link_libraries(dma_test m)
# target_link_options(dma_test PRIVATE -static)
