cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)

project(RDMA_FILTER)

option(TOGGLE_RDMA "enable rdma" ON)
option(TOGGLE_LOCK_FREE "enable lock-free" OFF)
option(TOGGLE_HUGEPAGE "enable hugepage allocation" OFF)
set(SWITCH_EXP "rdma_cf" CACHE STRING "rdma_bf/bbf/ohbbf/cf")
set_property(CACHE SWITCH_EXP PROPERTY STRINGS "rdma_bf" "rdma_bbf" "rdma_ohbbf" "rdma_cf")

add_subdirectory(src)
add_subdirectory(test)
