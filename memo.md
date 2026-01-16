cd D:\study\rdma-filter\

实验室机器
scp -r -i "C:\Users\Yuandu\.ssh\id_rsa" -P 10131 src test CMakeLists.txt build root@210.28.134.155:liuyunchuan/exp01
ssh -i "C:\Users\Yuandu\.ssh\id_rsa" -p 10131 root@210.28.134.155
cd liuyunchuan/exp01/build
cmake -DTOGGLE_RDMA=OFF ..
./test/1_test

美国机器

python scripts/test.py init
python scripts/test.py compile
python scripts/test.py deploy; python scripts/test.py run
python scripts/test.py collect
python scripts/test.py stop
python scripts/test.py compile -DTOGGLE_LOCK_FREE=ON
python scripts/test.py perftest

ssh -o StrictHostKeyChecking=no -i "C:\Users\Yuandu\.ssh\id_rsa" yunchuan@clnode352.clemson.cloudlab.us
ssh -o StrictHostKeyChecking=no -i "C:\Users\Yuandu\.ssh\id_rsa" yunchuan@clnode363.clemson.cloudlab.us
ssh -o StrictHostKeyChecking=no -i "C:\Users\Yuandu\.ssh\id_rsa" yunchuan@clnode392.clemson.cloudlab.us

scp -r src test build CMakeLists.txt yunchuan@ms0902.utah.cloudlab.us:exp01
scp -r src test build CMakeLists.txt yunchuan@ms0913.utah.cloudlab.us:exp01
scp -r src test build CMakeLists.txt yunchuan@ms0938.utah.cloudlab.us:exp01

mkdir exp01
sudo apt update
sudo apt install cmake libibverbs-dev rdma-core librdmacm1 librdmacm-dev ibverbs-utils infiniband-diags perftest linux-tools-common linux-tools-generic linux-cloud-tools-generic tmux

ib_send_bw -d mlx4_0 -i 2
ib_send_bw -d mlx4_0 -i 2               10.10.1.1
ib_send_bw -d mlx4_0 -i 2 -D 4 -s 65536 10.10.1.1

tmux new -s exp_srv
tmux new -s exp_cli
./exp1/build/test/2_srv > out.log 2>&1; echo done > done.flag
./exp1/build/test/2_cli > out.log 2>&1; echo done > done.flag
tmux new-session -d -s exp_srv './exp1/build/test/2_srv > out.log 2>&1 && echo done > done.flag'
tmux new-session -d -s exp_cli './exp1/build/test/2_cli > out.log 2>&1 && echo done > done.flag'


tmux new -s t1
[Ctrl]+[b] then [d]
tmux ls
tmux attach -t t1
tmux kill-session -t t1

ping 10.10.1.1


查看NAT地址
ip addr

查看网卡名
ibstat
或
ib_devinfo


## CloudLab Node

网卡支持rdma的节点

+ (Powder)
d760p, d760-gpu, d760-hgpu

+ Apt: 1
r320

+ CloudLab Utah: 5
m510, xl170, d6515, c6525, c6620 (d750, d7615, d760, d760-hbm)

+ Wisconsin: 5
c240g5, sm110p, sm220u, d7525, d8545

+ Clemson: 6
ibm8335, r7525, r650, r6525, nvidiagh, r6615






# DEBUG


报错：`RDMA READ failed: Work Request Flushed Error`
解决：填wq的远端内存地址越界了


报错：服务端listen时，客户端connect函数阻塞了
解决：调试出报错信息：bind failed: Address already in use，原因是上次运行没正常结束，没有释放系统socket
    连续运行两种rdma索引就会卡住，就是因为这个，不知道咋办，就一次只运行一次
    临时解决办法：`sudo lsof -i :18515`查看进程编号，然后`kill`


异常：使用从cuckoo filter库抄来的生成随机数的函数，生成随机数据集，做dram实验时发现fpr比预期高了一倍还多，然后使用固定的数字作数据集，fpr顺利降了下来，因此怀疑生成随机数据集的代码。
更新：使用固定的数字作数据集，fpr也不对，所以跟数据集没关系。把bf代码换成wormhole里的版本，fpr也不对，搁置了，找不出原因。
解决：师兄说是哈希函数的问题，不用在意。


报错：CAS lock failed: transport retry counter exceeded
    RDMA READ failed: transport retry counter exceeded
分析：本端发请求，对端没反应，本端又自动试了几次，一直没反应，重试次数就耗尽了，网卡就写一个失败标志放进cq里。
尝试：回退到之前无锁单客户端版本试一试，对比对比。
更新：在旧版本的基础上，为server添加创建锁列表、连接多client的功能，然后删去cq和qp，client基本没有改变，然后运行时就报了retry exceeded，因此怀疑是删去cq和qp的问题。
更新：再回退，只添加创建锁列表、连接多client的功能，不删去cq和qp，再次运行就正常。因此更确信是删去cq和qp的问题，不过暂不清楚原理。
定位问题：本来正常的代码，仅仅注释掉server里把qp.qp_num传给client的代码时，就出现了retry exceeded。但是还不了解原理。
原理：问题在于rdma连接必须要两边都有qp，即使被单边访问的一端不往qp里进行请求。rdma连接里一个机器的qp对应另一个机器的qp，所以有多个机器就要多个qp。所以server要给每个client创建一个qp，各自连接，不过cq可以不用多个，共用一个就行。已解决。


问题：误用cloudlab集群控制网络，流量过大被监测到，导致实验中断并收到官方邮件。
解决：网卡两个端口，第一个端口默认是控制网络的，所以要使用第二个端口。两个端口的GID表是独立的，要查一下确认GID的index，该index是较稳定的，一般不变。
如何判断该用哪个端口：对于ip地址，要用内网地址（10或者192.168）而不是公网地址。对于网卡端口，down状态的没开就不管，开着的里面，一种是控制用的，一种是实验用的。判断方法一：看性能，比如max MTU更大的是可能实验用的，rate更大的可能是实验用的；方法二，看mac地址，跟内网地址配对的那个mac地址，对应的是实验端口，另外实验cloudlab页面的manifest里面也能看到mac地址，那就是实验端口的。
三个指令：ip addr; ibstat; ibv_devinfo


问题：c6525-25g节点，用不了perftest
备注：不知道为啥，这个节点的rdma似乎有问题，后来没再试过，不用，用别的
