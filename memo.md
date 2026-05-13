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

python scripts/test.py compile -DSWITCH_EXP=rdma_bf
python scripts/test.py compile -DSWITCH_EXP=rdma_bbf
python scripts/test.py compile -DSWITCH_EXP=rdma_ohbbf
python scripts/test.py compile -DSWITCH_EXP=rdma_cf
python scripts/test.py compile -DSWITCH_EXP=rdma_cf -DTOGGLE_LOCK_FREE=ON
python scripts/test.py compile -DSWITCH_EXP=rdma_cf -DTOGGLE_HUGEPAGE=ON
python scripts/test.py perftest

ssh -o StrictHostKeyChecking=no -i "C:\Users\Yuandu\.ssh\id_rsa" yunchuan@clnode368.clemson.cloudlab.us
ssh -o StrictHostKeyChecking=no -i "C:\Users\Yuandu\.ssh\id_rsa" yunchuan@clnode365.clemson.cloudlab.us
ssh -o StrictHostKeyChecking=no -i "C:\Users\Yuandu\.ssh\id_rsa" yunchuan@clnode392.clemson.cloudlab.us

scp -r src test build CMakeLists.txt yunchuan@ms0902.utah.cloudlab.us:exp01
scp -r src test build CMakeLists.txt yunchuan@ms0913.utah.cloudlab.us:exp01
scp -r src test build CMakeLists.txt yunchuan@ms0938.utah.cloudlab.us:exp01

mkdir exp01
sudo apt update
sudo apt install cmake libibverbs-dev rdma-core librdmacm1 librdmacm-dev ibverbs-utils infiniband-diags perftest linux-tools-common linux-tools-generic linux-cloud-tools-generic tmux

ib_send_bw -d mlx5_0 -i 1 -s 64
ib_send_bw -d mlx5_0 -i 1 10.10.1.1 -s 64
ib_send_lat -d mlx5_0 -i 1 -s 64
ib_send_lat -d mlx5_0 -i 1 10.10.1.1 -s 64
ib_send_bw -d mlx5_0 -i 1 -s 8192
ib_send_bw -d mlx5_0 -i 1 10.10.1.1 -s 8192
ib_send_lat -d mlx5_0 -i 1 -s 8192
ib_send_lat -d mlx5_0 -i 1 10.10.1.1 -s 8192
-s 64 128 256 512 1024 2048 4096 8192
ping 10.10.1.1

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


查看NAT地址
ip addr

查看网卡名
ibstat
或
ibv_devinfo


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
解决：网卡多个端口，有一个端口默认是控制网络的，有的端口是给实验用的，有的端口是关闭的，所以要使用实验端口。不同端口的GID表是独立的，要查一下确认GID的index，该index是较稳定的，一般不变。
如何判断该用哪个端口：对于ip地址，要用内网地址（10或者192.168）而不是公网地址。对于网卡端口，down状态的没开就不管，开着的里面，一种是控制用的，一种是实验用的。判断方法一：看性能，比如max MTU更大的可能是实验用的，rate更大的可能是实验用的；方法二，看mac地址，跟内网地址配对的那个mac地址，对应的是实验端口，另外实验cloudlab页面的manifest里面也能看到mac地址，那就是实验端口的。
三个指令：ip addr; ibstat; ibv_devinfo


问题：c6525-25g节点，用不了perftest
备注：不知道为啥，这个节点的rdma似乎有问题，后来没再试过，不用，用别的


问题：跑 bbf 发现假阳率比 bf 高了几倍
猜测：猜是因为哈希的随机程度不够理想，block 不够随机平均。



# 思考

rdma 设计模式比如 qp ，cq 等等，都是异步思维，然而现在我在把他当成同步在用。我觉得可以使用协程，deepseek 建议协程的话使用协程池的模式。

rdma 读 cq 有两种方式，一种自旋轮询，第二种我之前不知道，是阻塞，等网卡填cq的时候叫醒。下面是 deepseek 给的一个代码示例。
```c
#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// 假设全局已有：
// struct ibv_context *ctx;      // 设备上下文
// struct ibv_cq *cq;            // 已经创建好的CQ
// struct ibv_qp *qp;            // 已经创建好的QP

// 本函数会阻塞等待CQ中有完成项，然后处理它
void wait_for_completion_and_poll(struct ibv_cq *cq, struct ibv_comp_channel *channel) {
    struct ibv_cq *ev_cq;      // 收到事件时，会填上哪个CQ发生了事件
    void *ev_ctx;              // 事件上下文（创建CQ时可设置，这里没用到）
    struct ibv_wc wc;          // 用于存放一个完成条目（Completion Entry）
    int ret;

    // ===== 步骤1：请求一个通知 =====
    // 告诉硬件：下次CQ有新的完成时，请通过 completion channel 发一个事件通知我
    // 参数0表示“下次任何类型的完成”都通知（也可以设1表示只在CQ从空变非空时通知）
    if (ibv_req_notify_cq(cq, 0)) {
        fprintf(stderr, "ibv_req_notify_cq failed\n");
        return;
    }

    // ===== 步骤2：阻塞等待网卡事件 =====
    // 这个函数会阻塞当前线程，直到网卡通过 completion channel 发送一个事件。
    // 当网卡把完成项写入CQ时，它会同时通过channel发信号。
    if (ibv_get_cq_event(channel, &ev_cq, &ev_ctx)) {
        fprintf(stderr, "ibv_get_cq_event failed\n");
        return;
    }

    // 到达这里说明：网卡已经通知我们有完成项了。
    // 注意：ibv_get_cq_event 只会告诉我们“至少有一个完成项”，但不会告诉我们具体有多少个。
    // 所以我们接下来需要去轮询CQ，取出所有已经完成的项。

    // ===== 步骤3：确认这个事件（必须调用，否则后续事件可能不触发）=====
    // 参数：哪个CQ，确认多少个事件（通常1）
    ibv_ack_cq_events(cq, 1);

    // ===== 步骤4：不断轮询，把当前CQ中所有的完成项都取出来处理 =====
    // 注意：因为网卡可能在我们处理的同时又完成了新的操作，并写入了CQ，
    // 所以我们要一直轮询直到没有新的完成项为止。
    while (1) {
        // ibv_poll_cq 每次从CQ中取出一个完成项（如果存在），返回1，并把内容填入wc。
        // 如果CQ里没有完成项，返回0。
        // 如果出错，返回负数。
        ret = ibv_poll_cq(cq, 1, &wc);
        if (ret < 0) {
            fprintf(stderr, "ibv_poll_cq error\n");
            break;
        }
        if (ret == 0) {
            // CQ已经空了，退出循环
            break;
        }

        // 处理这个完成项
        if (wc.status != IBV_WC_SUCCESS) {
            // 操作失败
            fprintf(stderr, "Work completion error: %d (wr_id=%lu)\n", wc.status, (unsigned long)wc.wr_id);
            // 这里可以做一些错误处理，比如重新提交或标记失败
        } else {
            // 操作成功
            // wc.wr_id 是我们提交操作时设置的自定义ID，通常用于找到对应的请求上下文
            printf("Successful completion: wr_id=%lu, opcode=%d\n", 
                   (unsigned long)wc.wr_id, wc.opcode);
            // 根据 wr_id 找到之前提交的请求，处理数据等...
        }
    }

    // ===== 步骤5：重要！为了下一次能继续收到通知，需要再次调用 ibv_req_notify_cq =====
    // 注意：上面的 ibv_poll_cq 循环已经把所有完成项取干净了。
    // 如果我们不重新请求通知，下一次有新完成时网卡不会发事件，我们就永远阻塞在 ibv_get_cq_event 了。
    // 所以通常在循环的开头或结尾重新请求通知。这里我们放在函数末尾，但实际业务中往往是个无限循环。
    if (ibv_req_notify_cq(cq, 0)) {
        fprintf(stderr, "ibv_req_notify_cq (rearm) failed\n");
    }

    // 接下来你可以继续调用本函数，再次等待下一个完成事件。
    // 实际使用时，通常会写一个 while(1) { wait_and_process(...); } 循环。
}
```