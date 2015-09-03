CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(zupply)
SET(CMAKE_CXX_FLAGS "-std=c++11")
add_executable(zupply_sample
				../src/sample.cpp
				../src/zupply.hpp
				../src/zupply.cpp)