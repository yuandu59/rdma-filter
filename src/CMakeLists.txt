add_library(header INTERFACE)
target_include_directories(header INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/dram_bf
    ${CMAKE_CURRENT_SOURCE_DIR}/dram_bbf
    ${CMAKE_CURRENT_SOURCE_DIR}/dram_ohbbf
    ${CMAKE_CURRENT_SOURCE_DIR}/rdma_bf
    ${CMAKE_CURRENT_SOURCE_DIR}/rdma_bbf
    ${CMAKE_CURRENT_SOURCE_DIR}/rdma_ohbbf
)


add_library(murmur STATIC "common/murmur3.cpp")
target_link_libraries(murmur PUBLIC header)


add_library(drambf STATIC "dram_bf/dram_bf.cpp")
target_link_libraries(drambf PUBLIC header murmur)

add_library(drambbf STATIC "dram_bbf/dram_bbf.cpp")
target_link_libraries(drambbf PUBLIC header murmur)

add_library(dramohbbf STATIC "dram_ohbbf/dram_ohbbf.cpp")
target_link_libraries(dramohbbf PUBLIC header murmur)

add_library(rdmabf STATIC "rdma_bf/rdma_bf.cpp")
target_link_libraries(rdmabf PUBLIC header murmur)

add_library(rdmabbf STATIC "rdma_bbf/rdma_bbf.cpp")
target_link_libraries(rdmabbf PUBLIC header murmur)

add_library(rdmaohbbf STATIC "rdma_ohbbf/rdma_ohbbf.cpp")
target_link_libraries(rdmaohbbf PUBLIC header murmur)