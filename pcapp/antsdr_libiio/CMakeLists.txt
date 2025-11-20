cmake_minimum_required(VERSION 3.00)
project(antsdr_libiio)

set(CMAKE_CXX_STANDARD 17)

find_library(libiio_LIBRARY iio)
if(libiio_FOUND)
    message(STATUS "Find libiio library")
endif ()

find_package(Python3 COMPONENTS Development NumPy)
if(Python3_FOUND)
    message(STATUS "Python3 found: ${Python3_VERSION}")
    include_directories(${Python3_INCLUDE_DIRS} ${Python3_NumPy_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Python3 not found")
endif()

add_executable(antsdr_libiio main.cpp antsdrDevice.h antsdrDevice.cpp
        log.cpp
        math_utils.h
        math_utils.cpp)

target_link_libraries(antsdr_libiio iio ad9361 pthread fftw3f Python3::Python Python3::NumPy)
