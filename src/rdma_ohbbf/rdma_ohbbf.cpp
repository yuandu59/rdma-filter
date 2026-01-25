#include "rdma_ohbbf.h"
#include "murmur3.h"
#include "utils.h"

#include <iostream>
#include <math.h>
#include <string.h>
#include <unistd.h>

using namespace std;

#define bit_set(v, n) ((v)[(n) >> 3] |= (0x1 << (0x7 - ((n)&0x7))))
#define bit_get(v, n) ((v)[(n) >> 3] & (0x1 << (0x7 - ((n)&0x7))))
#define bit_clr(v, n) ((v)[(n) >> 3] &= ~(0x1 << (0x7 - ((n)&0x7))))

#define SEED (2568)


void RdmaOHBBF_Cli_init(struct RdmaOHBBF_Cli *rdma_ohbbf, unsigned int n, double fpr, unsigned int block_size, const char* server_ip, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index, uint32_t mutex_gran_block)
{
    memset(rdma_ohbbf, 0, sizeof(*rdma_ohbbf));

    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;
    rdma_ohbbf->m = ((int(m) >> 3) + 1) << 3;
    rdma_ohbbf->k = ceil(k);
    rdma_ohbbf->mutex_gran_block = mutex_gran_block;

    rdma_ohbbf->block_size = block_size;
    rdma_ohbbf->block_count = rdma_ohbbf->m / (block_size << 3);

    // 打开 ctx 和 注册 pd 和 cq
    rdma_ohbbf->ctx = open_rdma_ctx(name_dev);
    rdma_ohbbf->pd = ibv_alloc_pd(rdma_ohbbf->ctx);
    rdma_ohbbf->cq = ibv_create_cq(rdma_ohbbf->ctx, 16, NULL, NULL, 0);

    // 分配两个 buffer 内存
    rdma_ohbbf->local_buf = (uint8_t *)malloc(1);
    int ret_posix = posix_memalign((void**)&rdma_ohbbf->mutex_buf, 8, 8);
    assert_else(ret_posix == 0, "posix_memalign failed for mutex_buf");
    
    // 注册两个 mr
    rdma_ohbbf->local_mr = ibv_reg_mr(rdma_ohbbf->pd, rdma_ohbbf->local_buf, 1, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
    assert_else(rdma_ohbbf->local_mr != nullptr, "ibv_reg_mr for local_mr failed");

    rdma_ohbbf->mutex_mr = ibv_reg_mr(rdma_ohbbf->pd, rdma_ohbbf->mutex_buf, 8, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_ATOMIC);
    assert_else(rdma_ohbbf->mutex_mr != nullptr, "ibv_reg_mr for mutex_mr failed");
    // 注册两个 sge
    rdma_ohbbf->buffer_sge = create_sge(rdma_ohbbf->local_mr);
    rdma_ohbbf->mutex_sge = create_sge(rdma_ohbbf->mutex_mr);

    // 创建 qp 并改到 init 状态
    rdma_ohbbf->qp = create_rc_qp(rdma_ohbbf->pd, rdma_ohbbf->cq);
    modify_init_qp(rdma_ohbbf->qp, rnic_port);

    // 创建 local info
    rdma_conn_info *local_info = create_local_info(rdma_ohbbf->ctx, rnic_port, gid_index);
    local_info->qp_num = rdma_ohbbf->qp->qp_num;

    // 建立 tcp 连接
    rdma_ohbbf->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sock_addr = {};
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tcp_port);
    inet_pton(AF_INET, server_ip, &sock_addr.sin_addr);
    std::cout << "[Client] Connecting to server at " << server_ip << "..." << std::endl;
    int return_connect = 1;
    while (return_connect != 0) {
        return_connect = connect(rdma_ohbbf->sockfd, (sockaddr*)&sock_addr, sizeof(sock_addr));
        if (return_connect < 0) sleep(1);
    }
    std::cout << "[Client] Connected to server at " << server_ip << std::endl;
    // 交换信息
    rdma_ohbbf->remote_info = {};
    reliable_send(rdma_ohbbf->sockfd, local_info, sizeof(rdma_conn_info));
    reliable_recv(rdma_ohbbf->sockfd, &rdma_ohbbf->remote_info, sizeof(rdma_conn_info));

    // 设置 QP 到 RTR 再到 RTS
    modify_rtr_qp(rdma_ohbbf->qp, rdma_ohbbf->remote_info.qp_num, rdma_ohbbf->remote_info.psn, rdma_ohbbf->remote_info.gid, gid_index, rnic_port);
    modify_rts_qp(rdma_ohbbf->qp, local_info->psn);

    char cmd[6];
    reliable_recv(rdma_ohbbf->sockfd, cmd, 6);
    std::cout << "[Client] Initialization successfully!" << std::endl;
    return;
}

void RdmaOHBBF_Cli_destroy(struct RdmaOHBBF_Cli *rdma_ohbbf) {
    if (rdma_ohbbf->local_mr) ibv_dereg_mr(rdma_ohbbf->local_mr);
    if (rdma_ohbbf->local_buf) free(rdma_ohbbf->local_buf);
    if (rdma_ohbbf->mutex_mr) ibv_dereg_mr(rdma_ohbbf->mutex_mr);
    if (rdma_ohbbf->mutex_buf) free(rdma_ohbbf->mutex_buf);
    if (rdma_ohbbf->buffer_sge) free(rdma_ohbbf->buffer_sge);
    if (rdma_ohbbf->mutex_sge) free(rdma_ohbbf->mutex_sge);
    if (rdma_ohbbf->qp) ibv_destroy_qp(rdma_ohbbf->qp);
    if (rdma_ohbbf->cq) ibv_destroy_cq(rdma_ohbbf->cq);
    if (rdma_ohbbf->pd) ibv_dealloc_pd(rdma_ohbbf->pd);
    if (rdma_ohbbf->ctx) ibv_close_device(rdma_ohbbf->ctx);

    if (rdma_ohbbf->sockfd) close(rdma_ohbbf->sockfd);
}

int RdmaOHBBF_Cli_insert(struct RdmaOHBBF_Cli *rdma_ohbbf, uint64_t key) {

    uint32_t hash, hash_;
    murmur3_hash32(&key, 8, SEED, &hash);
    uint32_t block_index = hash % rdma_ohbbf->block_count;
    uint32_t block_offset = block_index * rdma_ohbbf->block_size;
    uint32_t mutex_index = block_index / rdma_ohbbf->mutex_gran_block;
    bool write_back = false;

#ifndef TOGGLE_LOCK_FREE
    // 加锁（使用RDMA CAS原子操作）
    rdma_atomic_cas(rdma_ohbbf->qp, 100, rdma_ohbbf->mutex_sge, rdma_ohbbf->cq, rdma_ohbbf->remote_info.mutex_addr + mutex_index * sizeof(uint64_t), rdma_ohbbf->remote_info.mutex_rkey, 0, 1);
#endif

    // RDMA READ
    rdma_one_side(rdma_ohbbf->qp, 1, rdma_ohbbf->buffer_sge, rdma_ohbbf->remote_info.remote_addr + block_offset, rdma_ohbbf->remote_info.rkey, IBV_WR_RDMA_READ);
    check_cq(rdma_ohbbf->cq, 1);

    // Modify in place
    for (int i = 0; i < rdma_ohbbf->k; ++i) {
        hash_ = (hash >> 16) ^ (hash << i);
        auto bit_offset = hash_ % (rdma_ohbbf->block_size << 3);
        if (!write_back && !bit_get(rdma_ohbbf->local_buf, bit_offset)) {
            write_back = true;
        }
        bit_set(rdma_ohbbf->local_buf, bit_offset);
    }

    // RDMA WRITE
    if (write_back) {
        rdma_one_side(rdma_ohbbf->qp, 2, rdma_ohbbf->buffer_sge, rdma_ohbbf->remote_info.remote_addr + block_offset, rdma_ohbbf->remote_info.rkey, IBV_WR_RDMA_WRITE);
        check_cq(rdma_ohbbf->cq, 2);
    }

#ifndef TOGGLE_LOCK_FREE
    // 解锁（不使用CAS原子操作）
    *(uint64_t*)rdma_ohbbf->mutex_buf = 0;  // 设置交换值为0（解锁）
    rdma_one_side(rdma_ohbbf->qp, 101, rdma_ohbbf->mutex_sge, rdma_ohbbf->remote_info.mutex_addr + mutex_index * sizeof(uint64_t), rdma_ohbbf->remote_info.mutex_rkey, IBV_WR_RDMA_WRITE);
    check_cq(rdma_ohbbf->cq, 101);
#endif

    return 1;
}

int RdmaOHBBF_Cli_lookup(struct RdmaOHBBF_Cli *rdma_ohbbf, uint64_t key) {

    uint32_t hash, hash_;
    murmur3_hash32(&key, 8, SEED, &hash);
    uint32_t block_index = hash % rdma_ohbbf->block_count;
    uint32_t block_offset = block_index * rdma_ohbbf->block_size;
    uint32_t mutex_index = block_index / rdma_ohbbf->mutex_gran_block;
    int flag_found = 1;

#ifndef TOGGLE_LOCK_FREE
    // 加锁（使用RDMA CAS原子操作）
    rdma_atomic_cas(rdma_ohbbf->qp, 100, rdma_ohbbf->mutex_sge, rdma_ohbbf->cq, rdma_ohbbf->remote_info.mutex_addr + mutex_index * sizeof(uint64_t), rdma_ohbbf->remote_info.mutex_rkey, 0, 1);
#endif

    // RDMA READ
    rdma_one_side(rdma_ohbbf->qp, 1, rdma_ohbbf->buffer_sge, rdma_ohbbf->remote_info.remote_addr + block_offset, rdma_ohbbf->remote_info.rkey, IBV_WR_RDMA_READ);
    check_cq(rdma_ohbbf->cq, 1);

    // Check bit
    for (int i = 0; i < rdma_ohbbf->k; ++i) {
        hash_ = (hash >> 16) ^ (hash << i);
        if (!bit_get(rdma_ohbbf->local_buf, hash_ % (rdma_ohbbf->block_size << 3))) {
            flag_found = 0;
            break;
        }
    }

#ifndef TOGGLE_LOCK_FREE
    // 解锁（不使用CAS原子操作）
    *(uint64_t*)rdma_ohbbf->mutex_buf = 0;  // 设置交换值为0（解锁）
    rdma_one_side(rdma_ohbbf->qp, 101, rdma_ohbbf->mutex_sge, rdma_ohbbf->remote_info.mutex_addr + mutex_index * sizeof(uint64_t), rdma_ohbbf->remote_info.mutex_rkey, IBV_WR_RDMA_WRITE);
    check_cq(rdma_ohbbf->cq, 101);
#endif

    return flag_found;
}

void RdmaOHBBF_Srv_init(struct RdmaOHBBF_Srv *rdma_ohbbf, unsigned int n, double fpr, int client_count, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index, uint32_t mutex_gran_block, unsigned int block_size) {
    memset(rdma_ohbbf, 0, sizeof(*rdma_ohbbf));

    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;
    rdma_ohbbf->m = ((int(m) >> 3) + 1) << 3;
    rdma_ohbbf->k = ceil(k);
    rdma_ohbbf->mutex_gran_block = mutex_gran_block;
    rdma_ohbbf->count_clients_expected = client_count;

#ifndef TOGGLE_LOCK_FREE
    uint32_t mutex_count = (rdma_ohbbf->m >> 3) / (block_size * mutex_gran_block) + 1;
#else
    uint32_t mutex_count = 1;
#endif

    // 初始化 bit_vector 和 mutex_list
    rdma_ohbbf->bit_vector = (uint8_t *)calloc(rdma_ohbbf->m >> 3, sizeof(uint8_t));
    int ret_posix = posix_memalign((void **)&rdma_ohbbf->mutex_list, 64, mutex_count * sizeof(uint64_t));
    assert_else(ret_posix == 0, "posix_memalign failed for mutex_list");
    memset(rdma_ohbbf->bit_vector, 0, rdma_ohbbf->m >> 3);
    memset(rdma_ohbbf->mutex_list, 0, mutex_count * sizeof(uint64_t));
    std::cout << "[Server] Data size(MB): " << rdma_ohbbf->m / 8 / 1024 / 1024 << std::endl;
    std::cout << "[Server] Mutex list size(KB): " << mutex_count * sizeof(uint64_t) / 1024 << std::endl;
    // 初始化 sockfd_list 和 remote_info_list
    rdma_ohbbf->list_sockfd = std::vector<int>(client_count);
    rdma_ohbbf->remote_info_list = (rdma_conn_info *)calloc(client_count, sizeof(rdma_conn_info));

    // 打开 ctx 并创建 pd 和 cq
    rdma_ohbbf->ctx = open_rdma_ctx(name_dev);
    rdma_ohbbf->pd = ibv_alloc_pd(rdma_ohbbf->ctx);
    rdma_ohbbf->cq = ibv_create_cq(rdma_ohbbf->ctx, 16, NULL, NULL, 0);

    // 注册两个 mr
    rdma_ohbbf->mr = ibv_reg_mr(rdma_ohbbf->pd, rdma_ohbbf->bit_vector, rdma_ohbbf->m >> 3,IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_LOCAL_WRITE);
    rdma_ohbbf->mutex_mr = ibv_reg_mr(rdma_ohbbf->pd, rdma_ohbbf->mutex_list, mutex_count * sizeof(uint64_t), IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_ATOMIC);
    assert_else(rdma_ohbbf->mr != nullptr, "ibv_reg_mr for bit_vector failed");
    assert_else(rdma_ohbbf->mutex_mr != nullptr, "ibv_reg_mr for mutex_mr failed");

    // 初始化 qp_list
    rdma_ohbbf->qp_list = (ibv_qp **)calloc(client_count, sizeof(ibv_qp *));
    for (int i = 0; i < client_count; ++i) {
        rdma_ohbbf->qp_list[i] = create_rc_qp(rdma_ohbbf->pd, rdma_ohbbf->cq);
        modify_init_qp(rdma_ohbbf->qp_list[i], rnic_port);
    }

    // 创建 local_info
    rdma_conn_info *local_info = create_local_info(rdma_ohbbf->ctx, rnic_port, gid_index);
    // 将地址和rkey加入连接信息中
    local_info->remote_addr = (uintptr_t)rdma_ohbbf->bit_vector;
    local_info->rkey = rdma_ohbbf->mr->rkey;
    local_info->mutex_addr = (uintptr_t)rdma_ohbbf->mutex_list;
    local_info->mutex_rkey = rdma_ohbbf->mutex_mr->rkey;

    // 开放 TCP 监听
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tcp_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    auto bind_result = bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
    assert_else(bind_result == 0, "Server bind TCP failed");
    std::cout << "[Server] Listening for client TCP connections..." << std::endl;

    auto listen_result = listen(listen_fd, client_count);
    assert_else(listen_result == 0, "Server listen TCP failed");

    // 建立连接 和 交换信息
    for (int i = 0; i < client_count; i++) {
        int client_fd = accept(listen_fd, NULL, NULL);
        assert_else(client_fd >= 0, "accept failed");
        rdma_ohbbf->list_sockfd[i] = client_fd;
        local_info->qp_num = rdma_ohbbf->qp_list[i]->qp_num;
        reliable_send(client_fd, local_info, sizeof(rdma_conn_info));
        reliable_recv(client_fd, &rdma_ohbbf->remote_info_list[i], sizeof(rdma_conn_info));

        std::cout << "[Server] connected client: " << i + 1 << '/' << client_count << std::endl;
    }

    // 修改 QP 到 RTR 再到 RTS
    for (int i = 0; i < client_count; i++) {
        modify_rtr_qp(rdma_ohbbf->qp_list[i], rdma_ohbbf->remote_info_list[i].qp_num, rdma_ohbbf->remote_info_list[i].psn, rdma_ohbbf->remote_info_list[i].gid, gid_index, rnic_port);
        modify_rts_qp(rdma_ohbbf->qp_list[i], local_info->psn);
    }

    for (int i = 0; i < client_count; i++) {
        reliable_send(rdma_ohbbf->list_sockfd[i], "READY", 6);
    }
    std::cout << "[Server] Initialization successfully!" << std::endl;
    return;
}

void RdmaOHBBF_Srv_destroy(struct RdmaOHBBF_Srv *rdma_ohbbf) {
    if (rdma_ohbbf->mr) ibv_dereg_mr(rdma_ohbbf->mr);
    if (rdma_ohbbf->mutex_mr) ibv_dereg_mr(rdma_ohbbf->mutex_mr);
    if (rdma_ohbbf->bit_vector) free(rdma_ohbbf->bit_vector);
    if (rdma_ohbbf->mutex_list) free(rdma_ohbbf->mutex_list);
    for (int i = 0; i < rdma_ohbbf->count_clients_expected; i++) {
        if (rdma_ohbbf->qp_list[i]) ibv_destroy_qp(rdma_ohbbf->qp_list[i]);
    }
    if (rdma_ohbbf->cq) ibv_destroy_cq(rdma_ohbbf->cq);
    if (rdma_ohbbf->pd) ibv_dealloc_pd(rdma_ohbbf->pd);
    if (rdma_ohbbf->ctx) ibv_close_device(rdma_ohbbf->ctx);

    if (rdma_ohbbf->qp_list) free(rdma_ohbbf->qp_list);
    for (int i = 0; i < rdma_ohbbf->count_clients_expected; i++) {
        if (rdma_ohbbf->list_sockfd[i]) close(rdma_ohbbf->list_sockfd[i]);
    }
    if (rdma_ohbbf->remote_info_list) free(rdma_ohbbf->remote_info_list);
}

void RdmaOHBBF_Srv_clear(struct RdmaOHBBF_Srv *rdma_ohbbf) {
    if (rdma_ohbbf->bit_vector) memset(rdma_ohbbf->bit_vector, 0, rdma_ohbbf->m >> 3);
}