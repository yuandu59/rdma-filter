#include "rdma_bf.h"
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

void RdmaBF_Cli_init(struct RdmaBF_Cli *rdma_bf, unsigned int n, double fpr, const char* server_ip)
{
    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;

    memset(rdma_bf, 0, sizeof(*rdma_bf));
    rdma_bf->m = ((int(m) >> 3) + 1) << 3;
    rdma_bf->k = ceil(k);

    // std::cout << "debug: 01" << std::endl;
    ibv_device **dev_list = ibv_get_device_list(NULL);
    rdma_bf->ctx = ibv_open_device(dev_list[0]);
    rdma_bf->pd = ibv_alloc_pd(rdma_bf->ctx);
    rdma_bf->cq = ibv_create_cq(rdma_bf->ctx, 16, NULL, NULL, 0);
    // std::cout << "debug: 02" << std::endl;
    ibv_qp_init_attr qp_init_attr = {};
    qp_init_attr.send_cq = rdma_bf->cq;
    qp_init_attr.recv_cq = rdma_bf->cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.cap.max_send_wr = 10;
    qp_init_attr.cap.max_recv_wr = 10;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;
    rdma_bf->qp = ibv_create_qp(rdma_bf->pd, &qp_init_attr);
    // std::cout << "debug: 03" << std::endl;
    ibv_qp_attr attr = {};
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = 1;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;
    ibv_modify_qp(rdma_bf->qp, &attr,
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);
    // std::cout << "debug: 04" << std::endl;
    rdma_conn_info local_info = {};
    local_info.qp_num = rdma_bf->qp->qp_num;
    local_info.psn = lrand48() & 0xffffff;
    // std::cout << "debug: 05" << std::endl;
    // 使用临时变量获取对齐的 GID
    ibv_gid tmp_gid;
    ibv_query_gid(rdma_bf->ctx, 1, GID_INDEX, &tmp_gid);
    // 拷贝到结构体成员，避免直接传非对齐指针
    memcpy(&local_info.gid, &tmp_gid, sizeof(ibv_gid));
    // std::cout << "debug: 06" << std::endl;
    // 连接到服务器交换信息
    rdma_bf->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, server_ip, &addr.sin_addr);
    connect(rdma_bf->sockfd, (sockaddr*)&addr, sizeof(addr));
    // std::cout << "debug: 07" << std::endl;
    rdma_bf->remote_info = {};
    send(rdma_bf->sockfd, &local_info, sizeof(local_info), 0);
    recv(rdma_bf->sockfd, &rdma_bf->remote_info, sizeof(rdma_bf->remote_info), 0);
    // close(sockfd);
    // std::cout << "debug: 08" << std::endl;
    // 设置 QP -> RTR
    ibv_qp_attr rtr_attr = {};
    rtr_attr.qp_state = IBV_QPS_RTR;
    rtr_attr.path_mtu = IBV_MTU_1024;
    rtr_attr.dest_qp_num = rdma_bf->remote_info.qp_num;
    rtr_attr.rq_psn = rdma_bf->remote_info.psn;
    rtr_attr.max_dest_rd_atomic = 1;
    rtr_attr.min_rnr_timer = 12;
    rtr_attr.ah_attr.is_global = 1;
    rtr_attr.ah_attr.grh.dgid = rdma_bf->remote_info.gid;
    rtr_attr.ah_attr.grh.sgid_index = GID_INDEX;
    rtr_attr.ah_attr.grh.hop_limit = 1;
    rtr_attr.ah_attr.port_num = 1;
    ibv_modify_qp(rdma_bf->qp, &rtr_attr,
        IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
        IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
        IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);
    // std::cout << "debug: 09" << std::endl;
    // 设置 QP -> RTS
    ibv_qp_attr rts_attr = {};
    rts_attr.qp_state = IBV_QPS_RTS;
    rts_attr.timeout = 14;
    rts_attr.retry_cnt = 7;
    rts_attr.rnr_retry = 7;
    rts_attr.sq_psn = local_info.psn;
    rts_attr.max_rd_atomic = 1;
    ibv_modify_qp(rdma_bf->qp, &rts_attr,
        IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC);
    // std::cout << "debug: 10" << std::endl;
    // 注册内存
    rdma_bf->local_buf = (uint8_t *)malloc(1);
    rdma_bf->local_mr = ibv_reg_mr(rdma_bf->pd, rdma_bf->local_buf, 1, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);

    if (!rdma_bf->local_mr) {
        fprintf(stderr, "ibv_reg_mr");
    }

    std::cout << "[Client] RDMA connection established successfully!" << std::endl;
    return;
}

void RdmaBF_Cli_destroy(struct RdmaBF_Cli *rdma_bf) {
    if (rdma_bf->local_mr) ibv_dereg_mr(rdma_bf->local_mr);
    if (rdma_bf->local_buf) free(rdma_bf->local_buf);
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

    // ---------------- RDMA READ ----------------
    ibv_sge sge = {};
    sge.addr = (uintptr_t)rdma_bf->local_buf;
    sge.length = 1;
    sge.lkey = rdma_bf->local_mr->lkey;

    ibv_send_wr wr = {};
    wr.wr_id = 1; // 用于 poll_cq 识别
    wr.opcode = IBV_WR_RDMA_READ;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = rdma_bf->remote_info.remote_addr + byte_offset;
    wr.wr.rdma.rkey = rdma_bf->remote_info.rkey;

    ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_bf->qp, &wr, &bad_wr)) {
        fprintf(stderr, "ibv_post_send (RDMA READ)");
        return 1;
    }

    // 等待完成事件（poll cq）
    ibv_wc wc;
    while (ibv_poll_cq(rdma_bf->cq, 1, &wc) < 1);
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "RDMA READ failed: %s\n", ibv_wc_status_str(wc.status));
        return 1;
    }

    // ---------------- Modify in place ----------------
    bit_set(rdma_bf->local_buf, bit_offset);

    // ---------------- RDMA WRITE ----------------
    sge.addr = (uintptr_t)rdma_bf->local_buf;
    sge.length = 1;
    sge.lkey = rdma_bf->local_mr->lkey;

    wr.wr_id = 2; // 用于 poll_cq 识别
    wr.opcode = IBV_WR_RDMA_WRITE;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = rdma_bf->remote_info.remote_addr + byte_offset;
    wr.wr.rdma.rkey = rdma_bf->remote_info.rkey;

    if (ibv_post_send(rdma_bf->qp, &wr, &bad_wr)) {
        fprintf(stderr, "ibv_post_send (RDMA WRITE)");
        return 1;
    }

    // 等待完成事件（poll cq）
    while (ibv_poll_cq(rdma_bf->cq, 1, &wc) < 1);
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "RDMA WRITE failed: %s\n", ibv_wc_status_str(wc.status));
    }

    }
    return 1;
}

int RdmaBF_Cli_lookup(struct RdmaBF_Cli *rdma_bf, uint64_t key) {

    for (int i = 0; i < rdma_bf->k; ++i) {
        uint32_t hash;
        murmur3_hash32(&key, 8, i, &hash);
        hash %= (rdma_bf->m);
        uint32_t byte_offset = hash >> 3;
        uint32_t bit_offset = hash & 7;

    // ---------- RDMA READ ----------
    ibv_sge sge = {};
    sge.addr = (uintptr_t)rdma_bf->local_buf;
    sge.length = 1;
    sge.lkey = rdma_bf->local_mr->lkey;

    ibv_send_wr wr = {};
    wr.wr_id = 1;
    wr.opcode = IBV_WR_RDMA_READ;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = rdma_bf->remote_info.remote_addr + byte_offset;
    wr.wr.rdma.rkey = rdma_bf->remote_info.rkey;

    ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_bf->qp, &wr, &bad_wr)) {
        fprintf(stderr, "ibv_post_send (lookup)");
        return 0;
    }

    // ---------- CQ轮询等待完成 ----------
    ibv_wc wc = {};
    while (ibv_poll_cq(rdma_bf->cq, 1, &wc) < 1);
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "RDMA READ failed in lookup: %s\n", ibv_wc_status_str(wc.status));
        return 0;
    }

    // ---------- Check bit ----------
    if (!bit_get(rdma_bf->local_buf, bit_offset)) {
        return 0;
    }

    }
    return 1;
}

void RdmaBF_Srv_init(struct RdmaBF_Srv *rdma_bf, unsigned int n, double fpr) {
    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;
    // std::cout << "debug: 01" << std::endl;
    memset(rdma_bf, 0, sizeof(*rdma_bf));
    rdma_bf->m = ((int(m) >> 3) + 1) << 3;
    rdma_bf->k = ceil(k);
    rdma_bf->bit_vector = (uint8_t *)calloc(rdma_bf->m >> 3, sizeof(uint8_t));
    // std::cout << "debug: 02" << std::endl;
    // 1. 获取 RDMA 设备和保护域
    ibv_device **dev_list = ibv_get_device_list(NULL);
    rdma_bf->ctx = ibv_open_device(dev_list[0]);
    rdma_bf->pd = ibv_alloc_pd(rdma_bf->ctx);
    // std::cout << "debug: 03" << std::endl;
    // 2. 创建 Completion Queue 和 Queue Pair
    rdma_bf->cq = ibv_create_cq(rdma_bf->ctx, 16, NULL, NULL, 0);
    ibv_qp_init_attr qp_init_attr = {};
    qp_init_attr.send_cq = rdma_bf->cq;
    qp_init_attr.recv_cq = rdma_bf->cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.cap.max_send_wr = 10;
    qp_init_attr.cap.max_recv_wr = 10;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;
    rdma_bf->qp = ibv_create_qp(rdma_bf->pd, &qp_init_attr);
    // std::cout << "debug: 04" << std::endl;
    // 3. 初始化 QP 到 INIT
    ibv_qp_attr attr = {};
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = 1;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;
    ibv_modify_qp(rdma_bf->qp, &attr,
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);
    // std::cout << "debug: 05" << std::endl;
    rdma_conn_info local_info = {};
    local_info.qp_num = rdma_bf->qp->qp_num;
    local_info.psn = lrand48() & 0xffffff;
    // std::cout << "debug: 06" << std::endl;
    // 注册内存区域
    rdma_bf->mr = ibv_reg_mr(rdma_bf->pd, rdma_bf->bit_vector, rdma_bf->m >> 3,
        IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_LOCAL_WRITE);

    if (!rdma_bf->mr) {
        fprintf(stderr, "ibv_reg_mr failed");
        return;
    }    
    // std::cout << "debug: 07" << std::endl;
    // 将地址和rkey加入连接信息中
    local_info.remote_addr = (uintptr_t)rdma_bf->bit_vector;
    local_info.rkey = rdma_bf->mr->rkey;

    // 4. 获取 GID
    // 使用临时变量获取对齐的 GID
    ibv_gid tmp_gid;
    ibv_query_gid(rdma_bf->ctx, 1, GID_INDEX, &tmp_gid);
    // 拷贝到结构体成员，避免直接传非对齐指针
    memcpy(&local_info.gid, &tmp_gid, sizeof(ibv_gid));
    // std::cout << "debug: 08" << std::endl;
    // 5. 建立 TCP 监听用于交换 GID 和 QPN
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        fprintf(stderr, "bind failed");
        exit(1);
    }

    std::cout << "[Server] Waiting for client TCP connection..." << std::endl;
    if (listen(listen_fd, 1) < 0) {
        fprintf(stderr, "listen failed");
        exit(1);
    }
    rdma_bf->sockfd = accept(listen_fd, NULL, NULL);
    // std::cout << "debug: 09" << std::endl;
    // 6. 交换连接信息
    rdma_conn_info remote_info = {};
    send(rdma_bf->sockfd, &local_info, sizeof(local_info), 0);
    recv(rdma_bf->sockfd, &remote_info, sizeof(remote_info), 0);

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
    ibv_modify_qp(rdma_bf->qp, &rtr_attr,
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
    ibv_modify_qp(rdma_bf->qp, &rts_attr,
        IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC);

    std::cout << "[Server] RDMA connection established successfully!" << std::endl;

    return;
}

void RdmaBF_Srv_destroy(struct RdmaBF_Srv *rdma_bf) {
    if (rdma_bf->mr) ibv_dereg_mr(rdma_bf->mr);
    if (rdma_bf->bit_vector) free(rdma_bf->bit_vector);
    if (rdma_bf->qp) ibv_destroy_qp(rdma_bf->qp);
    if (rdma_bf->cq) ibv_destroy_cq(rdma_bf->cq);
    if (rdma_bf->pd) ibv_dealloc_pd(rdma_bf->pd);
    if (rdma_bf->ctx) ibv_close_device(rdma_bf->ctx);

    if (rdma_bf->sockfd) close(rdma_bf->sockfd);
}

void RdmaBF_Srv_clear(struct RdmaBF_Srv *rdma_bf) {
    if (rdma_bf->bit_vector) memset(rdma_bf->bit_vector, 0, rdma_bf->m >> 3);
}