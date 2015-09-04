CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(zupply)
IF (WIN32)
    # FOR windows
ELSE()
    SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
ENDIF()
add_executable(zupply_sample
				../src/sample.cpp
				../src/zupply.hpp
				../src/zupply.cpp)