#ifndef __RDMA_BBF_H__
#define __RDMA_BBF_H__

#include <stdint.h>
#include <stdlib.h>

#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rdma_common.h"

struct RdmaBBF_Cli
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

void RdmaBBF_Cli_init(struct RdmaBBF_Cli *rdma_bbf, unsigned int n, double fpr, unsigned int block_size, const char* server_ip);

void RdmaBBF_Cli_destroy(struct RdmaBBF_Cli *rdma_bbf);

int RdmaBBF_Cli_insert(struct RdmaBBF_Cli *rdma_bbf, uint64_t key);

int RdmaBBF_Cli_lookup(struct RdmaBBF_Cli *rdma_bbf, uint64_t key);


struct RdmaBBF_Srv
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

void RdmaBBF_Srv_init(struct RdmaBBF_Srv *rdma_bbf, unsigned int n, double fpr);

void RdmaBBF_Srv_destroy(struct RdmaBBF_Srv *rdma_bbf);

void RdmaBBF_Srv_clear(struct RdmaBBF_Srv *rdma_bbf);

#endif /* __RDMA_BBF_H__ */