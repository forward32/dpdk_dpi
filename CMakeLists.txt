cmake_minimum_required(VERSION 2.8)

set(PRJ dpdk-dpi)
project(${PRJ})

file(GLOB SOURCES
  src/*.cpp
  src/protocols/*.cpp
  )

set(DPDK_RTE_TARGET x86_64-native-linuxapp-gcc)

include_directories(
  $ENV{RTE_SDK}/${DPDK_RTE_TARGET}/include
  src/
  )

link_directories($ENV{RTE_SDK}/${DPDK_RTE_TARGET}/lib)

set(DPDK_DRIVERS
  "-lrte_pmd_vmxnet3_uio -lrte_pmd_pcap"
  )

set(DPDK_LIBS
  "-Wl,--whole-archive"
  "-lrte_eal -lrte_mempool -lrte_mbuf -lrte_ring -lethdev -lrte_kvargs ${DPDK_DRIVERS}"
  "-Wl,--no-whole-archive"
  )

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall"
  )
add_definitions(-mssse3)

option(DPI_DEBUG "Debug mode (allow asserts, debug messages, etc)" OFF)
add_definitions(-DNDEBUG)
if(DPI_DEBUG)
  remove_definitions (-DNDEBUG)
endif(DPI_DEBUG)

add_executable(${PRJ} ${SOURCES})
add_subdirectory(test)
target_link_libraries(${PRJ} ${DPDK_LIBS})
target_link_libraries(${PRJ} pthread dl glog pcap)
