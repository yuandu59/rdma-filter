#ifndef __RDMA_BF_H__
#define __RDMA_BF_H__

#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rdma_common.h"

struct RdmaBF_Cli
{
    unsigned int m;
    unsigned int k;
    uint32_t mutex_gran;
    uint8_t *local_buf;
    uint64_t *mutex_buf;

    ibv_context *ctx;
    ibv_pd *pd;
    ibv_cq *cq;
    ibv_qp *qp;

    ibv_mr *local_mr;
    ibv_mr *mutex_mr;
    ibv_sge *buffer_sge;
    ibv_sge *mutex_sge;

    rdma_conn_info remote_info;

    int sockfd;
};

void RdmaBF_Cli_init(struct RdmaBF_Cli *rdma_bf, unsigned int n, double fpr, const char* server_ip, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index, uint32_t mutex_gran);

void RdmaBF_Cli_destroy(struct RdmaBF_Cli *rdma_bf);

int RdmaBF_Cli_insert(struct RdmaBF_Cli *rdma_bf, uint64_t key);

int RdmaBF_Cli_lookup(struct RdmaBF_Cli *rdma_bf, uint64_t key);


struct RdmaBF_Srv
{
    unsigned int m;
    unsigned int k;
    uint32_t mutex_gran;
    uint8_t *bit_vector;

    ibv_context *ctx;
    ibv_pd *pd;
    ibv_cq *cq;
    ibv_qp **qp_list;
    ibv_mr *mr;
    ibv_mr *mutex_mr;

    std::vector<int> list_sockfd;
    int count_clients_expected;
    rdma_conn_info *remote_info_list;

    uint64_t *mutex_list;
};

void RdmaBF_Srv_init(struct RdmaBF_Srv *rdma_bf, unsigned int n, double fpr, int client_count, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index, uint32_t mutex_gran);

void RdmaBF_Srv_destroy(struct RdmaBF_Srv *rdma_bf);

void RdmaBF_Srv_clear(struct RdmaBF_Srv *rdma_bf);

// ibv_send_wr DEFAULT_RDMA_READ_WR;

#endif /* __RDMA_BF_H__ */