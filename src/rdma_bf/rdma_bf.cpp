#include "rdma_bf.h"
#include "murmur3.h"
#include "utils.h"

#include <iostream>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <bitset>

using namespace std;

#define bit_set(v, n) ((v)[(n) >> 3] |= (0x1 << (0x7 - ((n)&0x7))))
#define bit_get(v, n) ((v)[(n) >> 3] & (0x1 << (0x7 - ((n)&0x7))))
#define bit_clr(v, n) ((v)[(n) >> 3] &= ~(0x1 << (0x7 - ((n)&0x7))))


void RdmaBF_Cli_init(struct RdmaBF_Cli *rdma_bf, unsigned int n, double fpr, const char* server_ip, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index, uint32_t mutex_gran)
{
    memset(rdma_bf, 0, sizeof(*rdma_bf));

    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;
    rdma_bf->m = ((int(m) >> 3) + 1) << 3;
    rdma_bf->k = ceil(k);
    rdma_bf->mutex_gran = mutex_gran;

    // 打开 ctx 和 注册 pd 和 cq
    rdma_bf->ctx = open_rdma_ctx(name_dev);
    rdma_bf->pd = ibv_alloc_pd(rdma_bf->ctx);
    rdma_bf->cq = ibv_create_cq(rdma_bf->ctx, 16, NULL, NULL, 0);

    // 分配两个 buffer 内存
    rdma_bf->local_buf = (uint8_t *)malloc(1);
    int ret_posix = posix_memalign((void**)&rdma_bf->mutex_buf, 8, 8);
    assert_else(ret_posix == 0, "posix_memalign failed for mutex_buf");
    // 注册两个 mr
    rdma_bf->local_mr = ibv_reg_mr(rdma_bf->pd, rdma_bf->local_buf, 1, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
    assert_else(rdma_bf->local_mr != nullptr, "ibv_reg_mr for local_mr failed");
    rdma_bf->mutex_mr = ibv_reg_mr(rdma_bf->pd, rdma_bf->mutex_buf, 8, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_ATOMIC);
    assert_else(rdma_bf->mutex_mr != nullptr, "ibv_reg_mr for mutex_mr failed");
    // 注册两个 sge
    rdma_bf->buffer_sge = create_sge(rdma_bf->local_mr);
    rdma_bf->mutex_sge = create_sge(rdma_bf->mutex_mr);

    // 创建 qp 并改到 init 状态
    rdma_bf->qp = create_rc_qp(rdma_bf->pd, rdma_bf->cq);
    modify_init_qp(rdma_bf->qp, rnic_port);

    // 创建 local info
    rdma_conn_info *local_info = create_local_info(rdma_bf->ctx, rnic_port, gid_index);
    local_info->qp_num = rdma_bf->qp->qp_num;

    // 建立 tcp 连接
    rdma_bf->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sock_addr = {};
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tcp_port);
    inet_pton(AF_INET, server_ip, &sock_addr.sin_addr);
    std::cout << "[Client] Connecting to server at " << server_ip << "..." << std::endl;
    int return_connect = 1;
    while (return_connect != 0) {
        return_connect = connect(rdma_bf->sockfd, (sockaddr*)&sock_addr, sizeof(sock_addr));
        if (return_connect < 0) sleep(1);
    }
    std::cout << "[Client] Connected to server at " << server_ip << std::endl;
    // 交换信息
    rdma_bf->remote_info = {};
    reliable_send(rdma_bf->sockfd, local_info, sizeof(rdma_conn_info));
    reliable_recv(rdma_bf->sockfd, &rdma_bf->remote_info, sizeof(rdma_conn_info));

    // 设置 QP 到 RTR 再到 RTS
    modify_rtr_qp(rdma_bf->qp, rdma_bf->remote_info.qp_num, rdma_bf->remote_info.psn, rdma_bf->remote_info.gid, gid_index, rnic_port);
    modify_rts_qp(rdma_bf->qp, local_info->psn);
    
    char cmd[6];
    reliable_recv(rdma_bf->sockfd, cmd, 6);
    std::cout << "[Client] Initialization successfully!" << std::endl;
    return;
}

void RdmaBF_Cli_destroy(struct RdmaBF_Cli *rdma_bf) {
    if (rdma_bf->local_mr) ibv_dereg_mr(rdma_bf->local_mr);
    if (rdma_bf->local_buf) free(rdma_bf->local_buf);
    if (rdma_bf->mutex_mr) ibv_dereg_mr(rdma_bf->mutex_mr);
    if (rdma_bf->mutex_buf) free(rdma_bf->mutex_buf);
    if (rdma_bf->buffer_sge) free(rdma_bf->buffer_sge);
    if (rdma_bf->mutex_sge) free(rdma_bf->mutex_sge);
    if (rdma_bf->qp) ibv_destroy_qp(rdma_bf->qp);
    if (rdma_bf->cq) ibv_destroy_cq(rdma_bf->cq);
    if (rdma_bf->pd) ibv_dealloc_pd(rdma_bf->pd);
    if (rdma_bf->ctx) ibv_close_device(rdma_bf->ctx);

    if (rdma_bf->sockfd) close(rdma_bf->sockfd);
}

int RdmaBF_Cli_insert(struct RdmaBF_Cli *rdma_bf, uint64_t key) {

    for (int i = 0; i < rdma_bf->k; ++i) {
        uint32_t hash;
        murmur3_hash32(&key, 8, i, &hash);
        hash %= rdma_bf->m;
        uint32_t byte_offset = hash >> 3;
        uint32_t bit_offset = hash & 7;
        uint32_t mutex_offset = byte_offset / rdma_bf->mutex_gran;
        // std::cout << "k Round: " << i << " , byte_offset: " << byte_offset << " , bit_offset: " << bit_offset << std::endl;

#ifndef TOGGLE_LOCK_FREE
    // 加锁（使用RDMA CAS原子操作）
    rdma_atomic_cas(rdma_bf->qp, 100, rdma_bf->mutex_sge, rdma_bf->cq, rdma_bf->remote_info.mutex_addr + mutex_offset * sizeof(uint64_t), rdma_bf->remote_info.mutex_rkey, 0, 1);
#endif

    // RDMA READ
    rdma_one_side(rdma_bf->qp, 1, rdma_bf->buffer_sge, rdma_bf->remote_info.remote_addr + byte_offset, rdma_bf->remote_info.rkey, IBV_WR_RDMA_READ);
    check_cq(rdma_bf->cq, 1);

    // Modify in place
    if (!bit_get(rdma_bf->local_buf, bit_offset)) {
        bit_set(rdma_bf->local_buf, bit_offset);
        // RDMA WRITE
        rdma_one_side(rdma_bf->qp, 2, rdma_bf->buffer_sge, rdma_bf->remote_info.remote_addr + byte_offset, rdma_bf->remote_info.rkey, IBV_WR_RDMA_WRITE);
        check_cq(rdma_bf->cq, 2);
    }

#ifndef TOGGLE_LOCK_FREE
    // 解锁（不使用CAS原子操作）
    *(uint64_t*)rdma_bf->mutex_buf = 0;  // 设置交换值为0（解锁）
    rdma_one_side(rdma_bf->qp, 101, rdma_bf->mutex_sge, rdma_bf->remote_info.mutex_addr + mutex_offset * sizeof(uint64_t), rdma_bf->remote_info.mutex_rkey, IBV_WR_RDMA_WRITE);
    check_cq(rdma_bf->cq, 101);
#endif
    }
    return 1;
}

int RdmaBF_Cli_lookup(struct RdmaBF_Cli *rdma_bf, uint64_t key) {

    int flag_found = 1;
    for (int i = 0; i < rdma_bf->k; ++i) {
        uint32_t hash;
        murmur3_hash32(&key, 8, i, &hash);
        hash %= (rdma_bf->m);
        uint32_t byte_offset = hash >> 3;
        uint32_t bit_offset = hash & 7;
        uint32_t mutex_offset = byte_offset / rdma_bf->mutex_gran;
        // std::cout << "k Round: " << i << " , byte_offset: " << byte_offset << " , bit_offset: " << bit_offset << std::endl;

#ifndef TOGGLE_LOCK_FREE
    // 加锁（使用RDMA CAS原子操作）
    rdma_atomic_cas(rdma_bf->qp, 100, rdma_bf->mutex_sge, rdma_bf->cq, rdma_bf->remote_info.mutex_addr + mutex_offset * sizeof(uint64_t), rdma_bf->remote_info.mutex_rkey, 0, 1);
#endif

    // RDMA READ
    rdma_one_side(rdma_bf->qp, 1, rdma_bf->buffer_sge, rdma_bf->remote_info.remote_addr + byte_offset, rdma_bf->remote_info.rkey, IBV_WR_RDMA_READ);
    check_cq(rdma_bf->cq, 1);
    
    // debug: print 8 bits one by one
    // std::cout << "Local buffer: " << std::bitset<8>(rdma_bf->local_buf[0]) << std::endl;

    // Check bit
    if (!bit_get(rdma_bf->local_buf, bit_offset)) {
        flag_found = 0;
    }

#ifndef TOGGLE_LOCK_FREE
    // 解锁（不使用RDMA原子操作）
    *(uint64_t*)rdma_bf->mutex_buf = 0;  // 设置交换值为0（解锁）
    rdma_one_side(rdma_bf->qp, 101, rdma_bf->mutex_sge, rdma_bf->remote_info.mutex_addr + mutex_offset * sizeof(uint64_t), rdma_bf->remote_info.mutex_rkey, IBV_WR_RDMA_WRITE);
    check_cq(rdma_bf->cq, 101);
#endif

    if (!flag_found) break;
    }
    return flag_found;
}

void RdmaBF_Srv_init(struct RdmaBF_Srv *rdma_bf, unsigned int n, double fpr, int client_count, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index, uint32_t mutex_gran) {
    memset(rdma_bf, 0, sizeof(*rdma_bf));

    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;
    rdma_bf->m = ((int(m) >> 3) + 1) << 3;
    rdma_bf->k = ceil(k);
    rdma_bf->mutex_gran = mutex_gran;
    rdma_bf->count_clients_expected = client_count;

#ifndef TOGGLE_LOCK_FREE
    uint32_t count_mutex = (rdma_bf->m >> 3) / mutex_gran + 1;
#else
    uint32_t count_mutex = 1;
#endif

    // 初始化 bit_vector 和 mutex_list
    rdma_bf->bit_vector = (uint8_t *)calloc(rdma_bf->m >> 3, sizeof(uint8_t));
    int ret_posix = posix_memalign((void **)&rdma_bf->mutex_list, 64, count_mutex * sizeof(uint64_t));
    assert_else(ret_posix == 0, "posix_memalign failed for mutex_list");
    memset(rdma_bf->bit_vector, 0, rdma_bf->m >> 3);
    memset(rdma_bf->mutex_list, 0, count_mutex * sizeof(uint64_t));
    std::cout << "[Server] BF size(MB): " << rdma_bf->m / 8 / 1024 / 1024 << std::endl;
    std::cout << "[Server] Mutex list size(KB): " << count_mutex * sizeof(uint64_t) / 1024 << std::endl;
    // 初始化 list_sockfd 和 remote_info_list
    rdma_bf->list_sockfd = std::vector<int>(client_count);
    rdma_bf->remote_info_list = (rdma_conn_info *)calloc(client_count, sizeof(rdma_conn_info));

    // 打开 ctx 并创建 pd 和 cq
    rdma_bf->ctx = open_rdma_ctx(name_dev);
    rdma_bf->pd = ibv_alloc_pd(rdma_bf->ctx);
    rdma_bf->cq = ibv_create_cq(rdma_bf->ctx, 16, NULL, NULL, 0);
    
    // 注册两个 mr
    rdma_bf->mr = ibv_reg_mr(rdma_bf->pd, rdma_bf->bit_vector, rdma_bf->m >> 3, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_LOCAL_WRITE);
    rdma_bf->mutex_mr = ibv_reg_mr(rdma_bf->pd, rdma_bf->mutex_list, count_mutex * sizeof(uint64_t), IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_ATOMIC);
    assert_else(rdma_bf->mr != nullptr, "ibv_reg_mr for bit_vector failed");
    assert_else(rdma_bf->mutex_mr != nullptr, "ibv_reg_mr for mutex_mr failed");
    
    // 初始化 qp_list
    rdma_bf->qp_list = (ibv_qp **)calloc(client_count, sizeof(ibv_qp *));
    for (int i = 0; i < client_count; ++i) {
        rdma_bf->qp_list[i] = create_rc_qp(rdma_bf->pd, rdma_bf->cq);
        modify_init_qp(rdma_bf->qp_list[i], rnic_port);
    }

    // 创建 local_info
    rdma_conn_info *local_info = create_local_info(rdma_bf->ctx, rnic_port, gid_index);
    // 将地址和rkey加入连接信息中
    local_info->remote_addr = (uintptr_t)rdma_bf->bit_vector;
    local_info->rkey = rdma_bf->mr->rkey;
    local_info->mutex_addr = (uintptr_t)rdma_bf->mutex_list;
    local_info->mutex_rkey = rdma_bf->mutex_mr->rkey;

    // 开放 TCP 监听
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tcp_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    auto bind_result = bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
    assert_else(bind_result == 0, "bind failed");
    std::cout << "[Server] Waiting for " << client_count << " client TCP connections..." << std::endl;

    auto listen_result = listen(listen_fd, client_count);
    assert_else(listen_result == 0, "listen failed");

    // 建立连接 和 交换信息
    for (int i = 0; i < client_count; i++) {
        int client_fd = accept(listen_fd, NULL, NULL);
        assert_else(client_fd >= 0, "accept failed");
        rdma_bf->list_sockfd[i] = client_fd;
        local_info->qp_num = rdma_bf->qp_list[i]->qp_num;
        reliable_send(client_fd, local_info, sizeof(rdma_conn_info));
        reliable_recv(client_fd, &rdma_bf->remote_info_list[i], sizeof(rdma_conn_info));

        std::cout << "[Server] connected client: " << i + 1 << '/' << client_count << std::endl;
    }

    // 修改 QP 到 RTR 再到 RTS
    for (int i = 0; i < client_count; i++) {
        modify_rtr_qp(rdma_bf->qp_list[i], rdma_bf->remote_info_list[i].qp_num, rdma_bf->remote_info_list[i].psn, rdma_bf->remote_info_list[i].gid, gid_index, rnic_port);
        modify_rts_qp(rdma_bf->qp_list[i], local_info->psn);
    }

    for (int i = 0; i < client_count; i++) {
        reliable_send(rdma_bf->list_sockfd[i], "READY", 6);
    }
    std::cout << "[Server] Initialization successfully!" << std::endl;
    return;
}

void RdmaBF_Srv_destroy(struct RdmaBF_Srv *rdma_bf) {
    if (rdma_bf->mr) ibv_dereg_mr(rdma_bf->mr);
    if (rdma_bf->mutex_mr) ibv_dereg_mr(rdma_bf->mutex_mr);
    if (rdma_bf->bit_vector) free(rdma_bf->bit_vector);
    if (rdma_bf->mutex_list) free(rdma_bf->mutex_list);
    for (int i = 0; i < rdma_bf->count_clients_expected; i++) {
        if (rdma_bf->qp_list[i]) ibv_destroy_qp(rdma_bf->qp_list[i]);
    }
    if (rdma_bf->cq) ibv_destroy_cq(rdma_bf->cq);
    if (rdma_bf->pd) ibv_dealloc_pd(rdma_bf->pd);
    if (rdma_bf->ctx) ibv_close_device(rdma_bf->ctx);

    if (rdma_bf->qp_list) free(rdma_bf->qp_list);
    for (int i = 0; i < rdma_bf->count_clients_expected; i++) {
        if (rdma_bf->list_sockfd[i]) close(rdma_bf->list_sockfd[i]);
    }
    if (rdma_bf->remote_info_list) free(rdma_bf->remote_info_list);
}

void RdmaBF_Srv_clear(struct RdmaBF_Srv *rdma_bf) {
    if (rdma_bf->bit_vector) memset(rdma_bf->bit_vector, 0, rdma_bf->m >> 3);
}