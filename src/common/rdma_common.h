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
    uint32_t psn;           // initial packet sequence number
    union ibv_gid gid;
    uint64_t remote_addr;   // server端位图地址
    uint32_t rkey;          // memory region key
    uint64_t mutex_addr;    // server端mutex地址
    uint32_t mutex_rkey;    // mutex memory region key
} __attribute__((packed));

// ibv_qp_init_attr DEFAULT_QP_INIT_ATTR;

// ibv_qp_attr DEFAULT_QP_ATTR;

// ibv_qp_attr DEFAULT_RTR_QP_ATTR;

// ibv_qp_attr DEFAULT_RTS_QP_ATTR;

#endif /* __RDMA_COMMON_H__ */