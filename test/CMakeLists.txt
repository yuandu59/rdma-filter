
add_executable(1_test "1_temp.cpp")
target_link_libraries(1_test PRIVATE
    header
    murmur
    drambf
    drambbf
    dramohbbf
)

add_executable(2_cli "2_cli.cpp")
target_link_libraries(2_cli PRIVATE
    header
    murmur
    rdmabf
    rdmabbf
    rdmaohbbf
    ibverbs
)

add_executable(2_srv "2_srv.cpp")
target_link_libraries(2_srv PRIVATE
    header
    murmur
    rdmabf
    rdmabbf
    rdmaohbbf
    ibverbs
)