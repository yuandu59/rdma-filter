
add_executable(1_test "1_dram.cpp")
target_link_libraries(1_test PRIVATE
    header
    murmur
    utils
    drambf
    drambbf
    dramohbbf
)
target_compile_options(1_test PRIVATE -Ofast)

add_executable(9_test "9_test.cpp")
target_compile_options(9_test PRIVATE -Ofast)

if (TOGGLE_RDMA)

    add_executable(0_test "0_rnic_check.cpp")
    target_link_libraries(0_test PRIVATE
        header
        murmur
        rdma_common
        utils
        ibverbs
    )
    target_compile_options(0_test PRIVATE -Ofast)

    add_executable(2_cli "2_cli.cpp")
    target_link_libraries(2_cli PRIVATE
        header
        murmur
        rdma_common
        utils
        rdmabf
        rdmabbf
        rdmaohbbf
        ibverbs
        rdmacf
    )
    target_compile_options(2_cli PRIVATE -Ofast)

    add_executable(2_srv "2_srv.cpp")
    target_link_libraries(2_srv PRIVATE
        header
        murmur
        rdma_common
        utils
        rdmabf
        rdmabbf
        rdmaohbbf
        ibverbs
        rdmacf
    )
    target_compile_options(2_srv PRIVATE -Ofast)

    add_executable(3_cli_test "3_cli_test.cpp")
    target_link_libraries(3_cli_test PRIVATE
        header
        murmur
        rdma_common
        utils
        rdmabf
        rdmabbf
        rdmaohbbf
        ibverbs
    )
    target_compile_options(3_cli_test PRIVATE -Ofast)

    if(TOGGLE_LOCK_FREE)
        target_compile_definitions(0_test       PUBLIC TOGGLE_LOCK_FREE)
        target_compile_definitions(2_cli        PUBLIC TOGGLE_LOCK_FREE)
        target_compile_definitions(2_srv        PUBLIC TOGGLE_LOCK_FREE)
        target_compile_definitions(3_cli_test   PUBLIC TOGGLE_LOCK_FREE)
    endif()

endif()