cmake_minimum_required(VERSION 2.8)

find_package(GTest REQUIRED)

set(PRJ dpdk-dpi-test)
project(${PRJ})

set(SOURCES
  main.cpp
  config.cpp
  )

add_executable(${PRJ} ${SOURCES})
include_directories(${GTEST_INCLUDE_DIRS})
target_link_libraries(${PRJ} ${GTEST_BOTH_LIBRARIES} pthread glog)
