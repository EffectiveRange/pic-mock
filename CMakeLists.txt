cmake_minimum_required(VERSION 3.28)

project(pic-mock)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(FwVer)
include(MockPIC)
include(PICBuild)

add_library(basefw SHARED basefw/i2c_app.c)


