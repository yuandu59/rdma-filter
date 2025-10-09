#include "rdma_bf.h"
#include "rdma_bbf.h"
#include "rdma_ohbbf.h"

#include <iostream>

int main(int argc, char **argv) {
    char cmd[16];



    struct RdmaBF_Srv rdma_bf_srv;
    RdmaBF_Srv_init(&rdma_bf_srv, 1000000, (double)1.0 / 512);

    send(rdma_bf_srv.sockfd, "READY", 6, 0);

    recv(rdma_bf_srv.sockfd, cmd, 5, 0);

    RdmaBF_Srv_destroy(&rdma_bf_srv);

// ----------------------------------------------------------------------------------------------

    // struct RdmaBBF_Srv rdma_bbf_srv;
    // RdmaBBF_Srv_init(&rdma_bbf_srv, 1000000, (double)1.0 / 512);

    // send(rdma_bbf_srv.sockfd, "READY", 6, 0);

    // recv(rdma_bbf_srv.sockfd, cmd, 5, 0);

    // RdmaBBF_Srv_destroy(&rdma_bbf_srv);

// ----------------------------------------------------------------------------------------------

    // struct RdmaOHBBF_Srv rdma_ohbbf_srv;
    // RdmaOHBBF_Srv_init(&rdma_ohbbf_srv, 1000000, (double)1.0 / 512);

    // send(rdma_ohbbf_srv.sockfd, "READY", 6, 0);

    // recv(rdma_ohbbf_srv.sockfd, cmd, 5, 0);

    // RdmaOHBBF_Srv_destroy(&rdma_ohbbf_srv);

    return 0;
}