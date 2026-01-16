cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)

project(RDMA_FILTER)

option(TOGGLE_RDMA "enable rdma" ON)
option(TOGGLE_LOCK_FREE "enable lock-free" OFF)

add_subdirectory(src)
add_subdirectory(test)
