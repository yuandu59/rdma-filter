
ssh -i "C:\Users\Yuandu\.ssh\id_rsa" yunchuan@c220g1-030830.wisc.cloudlab.us
ssh -i "C:\Users\Yuandu\.ssh\id_rsa" yunchuan@clnode294.clemson.cloudlab.us

cd D:\study\rdma-filter\
scp -r src test CMakeLists.txt yunchuan@c220g1-030830.wisc.cloudlab.us:exp01
scp -r src test CMakeLists.txt yunchuan@clnode294.clemson.cloudlab.us:exp01

scp src\rdma_bf\rdma_bf.h yunchuan@clnode305.clemson.cloudlab.us:exp01\src\rdma_bf
scp test\2_cli.cpp test\2_srv.cpp yunchuan@clnode305.clemson.cloudlab.us:exp01\test
scp test\2_cli.cpp test\2_srv.cpp yunchuan@clnode294.clemson.cloudlab.us:exp01\test

sudo apt update
sudo apt install cmake libibverbs-dev rdma-core librdmacm1 librdmacm-dev ibverbs-utils infiniband-diags perftest linux-tools-common linux-tools-generic linux-cloud-tools-generic


./test/1_test
./test/2_srv
./test/2_cli


ib_send_bw -d mlx5_0
ib_send_bw -d mlx5_0               10.10.1.2
ib_send_bw -d mlx5_0 -D 4 -s 65536 10.10.1.2 



# DEBUG


报错：`RDMA READ failed: Work Request Flushed Error`
解决：填wq的远端内存地址越界了


报错：服务端listen时，客户端connect函数阻塞了
解决：调试出报错信息：bind failed: Address already in use，原因是上次运行没正常结束，没有释放系统socket
    连续运行两种rdma索引就会卡住，就是因为这个，不知道咋办，就一次只运行一次
    临时解决办法：`sudo lsof -i :18515`，然后`kill`