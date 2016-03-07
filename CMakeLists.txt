cmake_minimum_required(VERSION 2.8)

set(PRJ dpdk-dpi)
project(${PRJ})

set(SOURCES
  src/main.cpp
  )

set(DPDK_RTE_TARGET x86_64-native-linuxapp-gcc)

include_directories(
  $ENV{RTE_SDK}/${DPDK_RTE_TARGET}/include
  )

link_directories($ENV{RTE_SDK}/${DPDK_RTE_TARGET}/lib)

set(DPDK_LIB
  "-lrte_eal -lrte_mempool -lrte_ring"
  )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

add_executable(${PRJ} ${SOURCES})
target_link_libraries(${PRJ} ${DPDK_LIB})
target_link_libraries(${PRJ} pthread dl)
