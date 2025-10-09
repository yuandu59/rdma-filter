#ifndef __RDMA_OHBBF_H__
#define __RDMA_OHBBF_H__

#include <stdint.h>
#include <stdlib.h>

#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rdma_common.h"

struct RdmaOHBBF_Cli
{
    unsigned int m;
    unsigned int k;
    uint8_t *local_buf;
    unsigned int block_size;
    unsigned int block_count;

    ibv_context *ctx;
    ibv_pd *pd;
    ibv_cq *cq;
    ibv_qp *qp;

    ibv_mr *local_mr;

    rdma_conn_info remote_info;

    int sockfd;
};

void RdmaOHBBF_Cli_init(struct RdmaOHBBF_Cli *rdma_ohbbf, unsigned int n, double fpr, unsigned int block_size, const char* server_ip);

void RdmaOHBBF_Cli_destroy(struct RdmaOHBBF_Cli *rdma_ohbbf);

int RdmaOHBBF_Cli_insert(struct RdmaOHBBF_Cli *rdma_ohbbf, uint64_t key);

int RdmaOHBBF_Cli_lookup(struct RdmaOHBBF_Cli *rdma_ohbbf, uint64_t key);


struct RdmaOHBBF_Srv
{
    unsigned int m;
    unsigned int k;
    uint8_t *bit_vector;

    ibv_context *ctx;
    ibv_pd *pd;
    ibv_cq *cq;
    ibv_qp *qp;
    ibv_mr *mr;

    int sockfd;
};

void RdmaOHBBF_Srv_init(struct RdmaOHBBF_Srv *rdma_ohbbf, unsigned int n, double fpr);

void RdmaOHBBF_Srv_destroy(struct RdmaOHBBF_Srv *rdma_ohbbf);

void RdmaOHBBF_Srv_clear(struct RdmaOHBBF_Srv *rdma_ohbbf);

#endif /* __RDMA_OHBBF_H__ */