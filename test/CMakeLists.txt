cmake_minimum_required(VERSION 2.8)

find_package(GTest REQUIRED)

set(PRJ dpdk-dpi-test)
project(${PRJ})

file(GLOB SOURCES
    *.cpp
    protocols/*.cpp

    ../src/common.cpp
    ../src/protocols/*.cpp
    )

set(DPDK_LIBS
  "-Wl,--whole-archive"
  "-lrte_eal -lrte_mempool -lrte_mbuf -lrte_ring"
  "-Wl,--no-whole-archive"
  )

add_executable(${PRJ} ${SOURCES})
include_directories(${GTEST_INCLUDE_DIRS})
target_link_libraries(${PRJ} ${DPDK_LIBS})
target_link_libraries(${PRJ} ${GTEST_BOTH_LIBRARIES} pthread glog dl)
