#include "rdma_ohbbf.h"
#include "murmur3.h"

#include <iostream>
#include <math.h>
#include <string.h>
#include <unistd.h>

using namespace std;

#define bit_set(v, n) ((v)[(n) >> 3] |= (0x1 << (0x7 - ((n)&0x7))))
#define bit_get(v, n) ((v)[(n) >> 3] & (0x1 << (0x7 - ((n)&0x7))))
#define bit_clr(v, n) ((v)[(n) >> 3] &= ~(0x1 << (0x7 - ((n)&0x7))))

#define TCP_PORT 18515       // 端口号用于 TCP 元数据交换
#define GID_INDEX 3
#define SEED 2568

void RdmaOHBBF_Cli_init(struct RdmaOHBBF_Cli *rdma_ohbbf, unsigned int n, double fpr, unsigned int block_size, const char* server_ip)
{
    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;

    memset(rdma_ohbbf, 0, sizeof(*rdma_ohbbf));
    rdma_ohbbf->m = ((int(m) >> 3) + 1) << 3;
    rdma_ohbbf->k = ceil(k);
    rdma_ohbbf->block_size = block_size;
    rdma_ohbbf->block_count = rdma_ohbbf->m / (rdma_ohbbf->block_size << 3);

    ibv_device **dev_list = ibv_get_device_list(NULL);
    rdma_ohbbf->ctx = ibv_open_device(dev_list[0]);
    rdma_ohbbf->pd = ibv_alloc_pd(rdma_ohbbf->ctx);
    rdma_ohbbf->cq = ibv_create_cq(rdma_ohbbf->ctx, 16, NULL, NULL, 0);

    ibv_qp_init_attr qp_init_attr = {};
    qp_init_attr.send_cq = rdma_ohbbf->cq;
    qp_init_attr.recv_cq = rdma_ohbbf->cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.cap.max_send_wr = 10;
    qp_init_attr.cap.max_recv_wr = 10;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;
    rdma_ohbbf->qp = ibv_create_qp(rdma_ohbbf->pd, &qp_init_attr);

    ibv_qp_attr attr = {};
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = 1;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;
    ibv_modify_qp(rdma_ohbbf->qp, &attr,
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

    rdma_conn_info local_info = {};
    local_info.qp_num = rdma_ohbbf->qp->qp_num;
    local_info.psn = lrand48() & 0xffffff;

    // 使用临时变量获取对齐的 GID
    ibv_gid tmp_gid;
    ibv_query_gid(rdma_ohbbf->ctx, 1, GID_INDEX, &tmp_gid);
    // 拷贝到结构体成员，避免直接传非对齐指针
    memcpy(&local_info.gid, &tmp_gid, sizeof(ibv_gid));

    // 连接到服务器交换信息
    rdma_ohbbf->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, server_ip, &addr.sin_addr);
    connect(rdma_ohbbf->sockfd, (sockaddr*)&addr, sizeof(addr));

    rdma_ohbbf->remote_info = {};
    send(rdma_ohbbf->sockfd, &local_info, sizeof(local_info), 0);
    recv(rdma_ohbbf->sockfd, &rdma_ohbbf->remote_info, sizeof(rdma_ohbbf->remote_info), 0);
    // close(sockfd);

    // 设置 QP -> RTR
    ibv_qp_attr rtr_attr = {};
    rtr_attr.qp_state = IBV_QPS_RTR;
    rtr_attr.path_mtu = IBV_MTU_1024;
    rtr_attr.dest_qp_num = rdma_ohbbf->remote_info.qp_num;
    rtr_attr.rq_psn = rdma_ohbbf->remote_info.psn;
    rtr_attr.max_dest_rd_atomic = 1;
    rtr_attr.min_rnr_timer = 12;
    rtr_attr.ah_attr.is_global = 1;
    rtr_attr.ah_attr.grh.dgid = rdma_ohbbf->remote_info.gid;
    rtr_attr.ah_attr.grh.sgid_index = GID_INDEX;
    rtr_attr.ah_attr.grh.hop_limit = 1;
    rtr_attr.ah_attr.port_num = 1;
    ibv_modify_qp(rdma_ohbbf->qp, &rtr_attr,
        IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
        IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
        IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);

    // 设置 QP -> RTS
    ibv_qp_attr rts_attr = {};
    rts_attr.qp_state = IBV_QPS_RTS;
    rts_attr.timeout = 14;
    rts_attr.retry_cnt = 7;
    rts_attr.rnr_retry = 7;
    rts_attr.sq_psn = local_info.psn;
    rts_attr.max_rd_atomic = 1;
    ibv_modify_qp(rdma_ohbbf->qp, &rts_attr,
        IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC);

    // 注册内存
    rdma_ohbbf->local_buf = (uint8_t *)malloc(block_size);
    rdma_ohbbf->local_mr = ibv_reg_mr(rdma_ohbbf->pd, rdma_ohbbf->local_buf, block_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);

    if (!rdma_ohbbf->local_mr) {
        fprintf(stderr, "ibv_reg_mr");
    }

    std::cout << "[Client] RDMA connection established successfully!" << std::endl;
    return;
}

void RdmaOHBBF_Cli_destroy(struct RdmaOHBBF_Cli *rdma_ohbbf) {
    if (rdma_ohbbf->local_mr) ibv_dereg_mr(rdma_ohbbf->local_mr);
    if (rdma_ohbbf->local_buf) free(rdma_ohbbf->local_buf);
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

    // ---------------- RDMA READ ----------------
    ibv_sge sge = {};
    sge.addr = (uintptr_t)rdma_ohbbf->local_buf;
    sge.length = rdma_ohbbf->block_size;
    sge.lkey = rdma_ohbbf->local_mr->lkey;

    ibv_send_wr wr = {};
    wr.wr_id = 1; // 用于 poll_cq 识别
    wr.opcode = IBV_WR_RDMA_READ;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = rdma_ohbbf->remote_info.remote_addr + block_offset;
    wr.wr.rdma.rkey = rdma_ohbbf->remote_info.rkey;

    ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_ohbbf->qp, &wr, &bad_wr)) {
        fprintf(stderr, "ibv_post_send (RDMA READ)");
        return 1;
    }

    // 等待完成事件（poll cq）
    ibv_wc wc;
    while (ibv_poll_cq(rdma_ohbbf->cq, 1, &wc) < 1);
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "RDMA READ failed: %s\n", ibv_wc_status_str(wc.status));
        return 1;
    }

    // ---------------- Modify in place ----------------
    for (int i = 0; i < rdma_ohbbf->k; ++i) {
        hash_ = (hash >> 16) ^ (hash << i);
        bit_set(rdma_ohbbf->local_buf, hash_ % (rdma_ohbbf->block_size << 3));
    }

    // ---------------- RDMA WRITE ----------------
    sge.addr = (uintptr_t)rdma_ohbbf->local_buf;
    sge.length = rdma_ohbbf->block_size;
    sge.lkey = rdma_ohbbf->local_mr->lkey;

    wr.wr_id = 2; // 用于 poll_cq 识别
    wr.opcode = IBV_WR_RDMA_WRITE;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = rdma_ohbbf->remote_info.remote_addr + block_offset;
    wr.wr.rdma.rkey = rdma_ohbbf->remote_info.rkey;

    if (ibv_post_send(rdma_ohbbf->qp, &wr, &bad_wr)) {
        fprintf(stderr, "ibv_post_send (RDMA WRITE)");
        return 1;
    }

    // 等待完成事件（poll cq）
    while (ibv_poll_cq(rdma_ohbbf->cq, 1, &wc) < 1);
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "RDMA WRITE failed: %s\n", ibv_wc_status_str(wc.status));
    }

    return 1;
}

int RdmaOHBBF_Cli_lookup(struct RdmaOHBBF_Cli *rdma_ohbbf, uint64_t key) {

    uint32_t hash, hash_;
    murmur3_hash32(&key, 8, SEED, &hash);
    uint32_t block_index = hash % rdma_ohbbf->block_count;
    uint32_t block_offset = block_index * rdma_ohbbf->block_size;

    // ---------- RDMA READ ----------
    ibv_sge sge = {};
    sge.addr = (uintptr_t)rdma_ohbbf->local_buf;
    sge.length = rdma_ohbbf->block_size;
    sge.lkey = rdma_ohbbf->local_mr->lkey;

    ibv_send_wr wr = {};
    wr.wr_id = 1;
    wr.opcode = IBV_WR_RDMA_READ;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = rdma_ohbbf->remote_info.remote_addr + block_offset;
    wr.wr.rdma.rkey = rdma_ohbbf->remote_info.rkey;

    ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_ohbbf->qp, &wr, &bad_wr)) {
        fprintf(stderr, "ibv_post_send (lookup)");
        return 0;
    }

    // ---------- CQ轮询等待完成 ----------
    ibv_wc wc = {};
    while (ibv_poll_cq(rdma_ohbbf->cq, 1, &wc) < 1);
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "RDMA READ failed in lookup: %s\n", ibv_wc_status_str(wc.status));
        return 0;
    }

    // ---------- Check bit ----------
    for (int i = 0; i < rdma_ohbbf->k; ++i) {
        hash_ = (hash >> 16) ^ (hash << i);
        if (!bit_get(rdma_ohbbf->local_buf, hash_ % (rdma_ohbbf->block_size << 3))) {
            return 0;
        }
    }
    return 1;
}

void RdmaOHBBF_Srv_init(struct RdmaOHBBF_Srv *rdma_ohbbf, unsigned int n, double fpr) {
    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;

    memset(rdma_ohbbf, 0, sizeof(*rdma_ohbbf));
    rdma_ohbbf->m = ((int(m) >> 3) + 1) << 3;
    rdma_ohbbf->k = ceil(k);
    rdma_ohbbf->bit_vector = (uint8_t *)calloc(rdma_ohbbf->m >> 3, sizeof(uint8_t));

    // 1. 获取 RDMA 设备和保护域
    ibv_device **dev_list = ibv_get_device_list(NULL);
    rdma_ohbbf->ctx = ibv_open_device(dev_list[0]);
    rdma_ohbbf->pd = ibv_alloc_pd(rdma_ohbbf->ctx);

    // 2. 创建 Completion Queue 和 Queue Pair
    rdma_ohbbf->cq = ibv_create_cq(rdma_ohbbf->ctx, 16, NULL, NULL, 0);
    ibv_qp_init_attr qp_init_attr = {};
    qp_init_attr.send_cq = rdma_ohbbf->cq;
    qp_init_attr.recv_cq = rdma_ohbbf->cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.cap.max_send_wr = 10;
    qp_init_attr.cap.max_recv_wr = 10;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;
    rdma_ohbbf->qp = ibv_create_qp(rdma_ohbbf->pd, &qp_init_attr);

    // 3. 初始化 QP 到 INIT
    ibv_qp_attr attr = {};
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = 1;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;
    ibv_modify_qp(rdma_ohbbf->qp, &attr,
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

    rdma_conn_info local_info = {};
    local_info.qp_num = rdma_ohbbf->qp->qp_num;
    local_info.psn = lrand48() & 0xffffff;

    // 注册内存区域
    rdma_ohbbf->mr = ibv_reg_mr(rdma_ohbbf->pd, rdma_ohbbf->bit_vector, rdma_ohbbf->m >> 3,
        IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_LOCAL_WRITE);

    if (!rdma_ohbbf->mr) {
        fprintf(stderr, "ibv_reg_mr failed");
        return;
    }    
    
    // 将地址和rkey加入连接信息中
    local_info.remote_addr = (uintptr_t)rdma_ohbbf->bit_vector;
    local_info.rkey = rdma_ohbbf->mr->rkey;

    // 4. 获取 GID
    // 使用临时变量获取对齐的 GID
    ibv_gid tmp_gid;
    ibv_query_gid(rdma_ohbbf->ctx, 1, GID_INDEX, &tmp_gid);
    // 拷贝到结构体成员，避免直接传非对齐指针
    memcpy(&local_info.gid, &tmp_gid, sizeof(ibv_gid));

    // 5. 建立 TCP 监听用于交换 GID 和 QPN
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd, (sockaddr*)&addr, sizeof(addr));

    std::cout << "[Server] Waiting for client TCP connection..." << std::endl;
    listen(listen_fd, 1);
    rdma_ohbbf->sockfd = accept(listen_fd, NULL, NULL);

    // 6. 交换连接信息
    rdma_conn_info remote_info = {};
    send(rdma_ohbbf->sockfd, &local_info, sizeof(local_info), 0);
    recv(rdma_ohbbf->sockfd, &remote_info, sizeof(remote_info), 0);

    // 7. 修改 QP 到 RTR
    ibv_qp_attr rtr_attr = {};
    rtr_attr.qp_state = IBV_QPS_RTR;
    rtr_attr.path_mtu = IBV_MTU_1024;
    rtr_attr.dest_qp_num = remote_info.qp_num;
    rtr_attr.rq_psn = remote_info.psn;
    rtr_attr.max_dest_rd_atomic = 1;
    rtr_attr.min_rnr_timer = 12;
    rtr_attr.ah_attr.is_global = 1;
    rtr_attr.ah_attr.grh.dgid = remote_info.gid;
    rtr_attr.ah_attr.grh.sgid_index = GID_INDEX;
    rtr_attr.ah_attr.grh.hop_limit = 1;
    rtr_attr.ah_attr.port_num = 1;
    ibv_modify_qp(rdma_ohbbf->qp, &rtr_attr,
        IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
        IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
        IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);

    // 8. 修改 QP 到 RTS
    ibv_qp_attr rts_attr = {};
    rts_attr.qp_state = IBV_QPS_RTS;
    rts_attr.timeout = 14;
    rts_attr.retry_cnt = 7;
    rts_attr.rnr_retry = 7;
    rts_attr.sq_psn = local_info.psn;
    rts_attr.max_rd_atomic = 1;
    ibv_modify_qp(rdma_ohbbf->qp, &rts_attr,
        IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC);

    std::cout << "[Server] RDMA connection established successfully!" << std::endl;

    return;
}

void RdmaOHBBF_Srv_destroy(struct RdmaOHBBF_Srv *rdma_ohbbf) {
    if (rdma_ohbbf->mr) ibv_dereg_mr(rdma_ohbbf->mr);
    if (rdma_ohbbf->bit_vector) free(rdma_ohbbf->bit_vector);
    if (rdma_ohbbf->qp) ibv_destroy_qp(rdma_ohbbf->qp);
    if (rdma_ohbbf->cq) ibv_destroy_cq(rdma_ohbbf->cq);
    if (rdma_ohbbf->pd) ibv_dealloc_pd(rdma_ohbbf->pd);
    if (rdma_ohbbf->ctx) ibv_close_device(rdma_ohbbf->ctx);

    if (rdma_ohbbf->sockfd) close(rdma_ohbbf->sockfd);
}

void RdmaOHBBF_Srv_clear(struct RdmaOHBBF_Srv *rdma_ohbbf) {
    if (rdma_ohbbf->bit_vector) memset(rdma_ohbbf->bit_vector, 0, rdma_ohbbf->m >> 3);
}