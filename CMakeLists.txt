cmake_minimum_required(VERSION 2.8.0)

project(strsupport)

enable_testing()

find_package( Boost 1.54 COMPONENTS context unit_test_framework REQUIRED )

include_directories( ${Boost_INCLUDE_DIR} )
include_directories( ${strsupport_SOURCE_DIR} )

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

if (NOT COMPILER_SUPPORTS_CXX11)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

add_subdirectory(src)
add_subdirectory(test)

