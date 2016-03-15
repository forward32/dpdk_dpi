cmake_minimum_required(VERSION 2.8)

set(PRJ dpdk-dpi)
project(${PRJ})

set(SOURCES
  src/main.cpp
  src/packet_manager.cpp
  src/port_manager.cpp
  src/port.cpp
  )

set(DPDK_RTE_TARGET x86_64-native-linuxapp-gcc)

include_directories(
  $ENV{RTE_SDK}/${DPDK_RTE_TARGET}/include
  src/
  )

link_directories($ENV{RTE_SDK}/${DPDK_RTE_TARGET}/lib)

set(DPDK_DRIVERS
  "-lrte_pmd_vmxnet3_uio"
  )

set(DPDK_LIBS
  "-Wl,--whole-archive"
  "-lrte_eal -lrte_mempool -lrte_mbuf -lrte_ring -lethdev ${DPDK_DRIVERS}"
  "-Wl,--no-whole-archive"
  )

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall"
  )
add_definitions(-mssse3)

add_executable(${PRJ} ${SOURCES})
target_link_libraries(${PRJ} ${DPDK_LIBS})
target_link_libraries(${PRJ} pthread dl glog)
