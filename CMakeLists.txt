cmake_minimum_required(VERSION 3.28)

project(pic-mock)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(FwVer)

pic_fw_get_semver(FW_SEMVER)

message(STATUS "FW semver is ${FW_SEMVER}")