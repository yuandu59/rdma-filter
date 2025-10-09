#ifndef __RDMA_COMMON_H__
#define __RDMA_COMMON_H__

#include <stdint.h>
#include <stdlib.h>

#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

struct rdma_conn_info {
    uint32_t qp_num;
    uint32_t psn;
    union ibv_gid gid;
    uint64_t remote_addr;   // server端位图地址
    uint32_t rkey;          // memory region key
    ibv_cq *cq;             // completion queue, 用于 poll CQ
} __attribute__((packed));

#endif /* __RDMA_COMMON_H__ */