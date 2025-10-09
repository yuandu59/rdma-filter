#include "rdma_bf.h"
#include "rdma_bbf.h"
#include "rdma_ohbbf.h"

#include <iostream>

#define SERVER_IP "10.10.1.2"
#define INSERT_COUNT (1 << 26)
#define FALSE_POSITIVE_RATE (double)1.0 / 512
#define BLOCK_SIZE 4

int main(int argc, char **argv) {

    char cmd[16];
    int false_positive;
    std::cout << "== Experiment Begin ==" << std::endl;



    struct RdmaBF_Cli rdma_bf_cli;
    RdmaBF_Cli_init(&rdma_bf_cli, INSERT_COUNT, FALSE_POSITIVE_RATE, SERVER_IP);

    recv(rdma_bf_cli.sockfd, cmd, 6, 0);

    for (uint64_t i = 0; i < 10000; i++) {
        RdmaBF_Cli_insert(&rdma_bf_cli, i);
    }

    false_positive = 0;
    for (uint64_t i = 10000; i < 20000; i++) {
        if (!RdmaBF_Cli_lookup(&rdma_bf_cli, i)) {
            ;
        }
    }

    send(rdma_bf_cli.sockfd, "EXIT", 5, 0);
    RdmaBF_Cli_destroy(&rdma_bf_cli);

// ----------------------------------------------------------------------------------------------

    // struct RdmaBBF_Cli rdma_bbf_cli;
    // RdmaBBF_Cli_init(&rdma_bbf_cli, INSERT_COUNT, FALSE_POSITIVE_RATE, BLOCK_SIZE, SERVER_IP);

    // recv(rdma_bbf_cli.sockfd, cmd, 6, 0);

    // for (uint64_t i = 0; i < 10000; i++) {
    //     RdmaBBF_Cli_insert(&rdma_bbf_cli, i);
    // }

    // false_positive = 0;
    // for (uint64_t i = 10000; i < 20000; i++) {
    //     if (!RdmaBBF_Cli_lookup(&rdma_bbf_cli, i)) {
    //         ;
    //     }
    // }

    // send(rdma_bbf_cli.sockfd, "EXIT", 5, 0);
    // RdmaBBF_Cli_destroy(&rdma_bbf_cli);

// ----------------------------------------------------------------------------------------------

    // struct RdmaOHBBF_Cli rdma_ohbbf_cli;
    // RdmaOHBBF_Cli_init(&rdma_ohbbf_cli, INSERT_COUNT, FALSE_POSITIVE_RATE, BLOCK_SIZE, SERVER_IP);

    // recv(rdma_ohbbf_cli.sockfd, cmd, 6, 0);

    // for (uint64_t i = 0; i < 10000; i++) {
    //     RdmaOHBBF_Cli_insert(&rdma_ohbbf_cli, i);
    // }

    // false_positive = 0;
    // for (uint64_t i = 10000; i < 20000; i++) {
    //     if (!RdmaOHBBF_Cli_lookup(&rdma_ohbbf_cli, i)) {
    //         ;
    //     }
    // }

    // send(rdma_ohbbf_cli.sockfd, "EXIT", 5, 0);
    // RdmaOHBBF_Cli_destroy(&rdma_ohbbf_cli);

    return 0;
}