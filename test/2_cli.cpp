#include "rdma_bf.h"
#include "rdma_bbf.h"
#include "rdma_ohbbf.h"
#include "utils.h"

#include <iostream>
#include <unordered_set>
#include <chrono>


#define SERVER_IP "10.10.1.2"
#define INSERT_COUNT (1 << 26)
#define LOOKUP_COUNT (1 << 16)
#define FALSE_POSITIVE_RATE (double)1.0 / 512
#define BLOCK_SIZE 4

#define REAL_INSERT_COUNT (1 << 16)

int main(int argc, char **argv) {

    int false_positive_count = 0, true_positive_count = 0, true_negative_count = 0;
    std::vector<uint64_t> to_insert = GenerateRandom64(REAL_INSERT_COUNT), to_lookup = {};
    std::unordered_set<uint64_t> to_insert_set(to_insert.begin(), to_insert.end());
    for (uint64_t i = 0; to_lookup.size() < LOOKUP_COUNT; i++) {
        if (to_insert_set.find(i) == to_insert_set.end()) {
            to_lookup.push_back(i);
        }
    }
    std::cout << "== Experiment Begin ==" << std::endl;

    struct RdmaBF_Cli rdma_bf_cli;
    RdmaBF_Cli_init(&rdma_bf_cli, INSERT_COUNT, FALSE_POSITIVE_RATE, SERVER_IP);

    std::cout << "RdmaFilter Inserting " << REAL_INSERT_COUNT << " items..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    for (auto i : to_insert) {
        RdmaBF_Cli_insert(&rdma_bf_cli, i);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "RdmaFilter Inserted " << REAL_INSERT_COUNT << " items." <<  std::endl;
    std::cout << "Insert Time(s): " << duration.count() / 1000.0 << std::endl;
    std::cout << "Throughput-Insert(op/s): " << REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    std::cout << "Payload-Bandwidth-Insert(MB/s): " << REAL_INSERT_COUNT * 2 * rdma_bf_cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;

    std::cout << std::endl << "RdmaFilter lookup existing keys..." << std::endl;
    start_time = std::chrono::high_resolution_clock::now();
    for (auto i : to_insert) {
        if (RdmaBF_Cli_lookup(&rdma_bf_cli, i)) {
            true_positive_count++;
        }
    }
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "RdmaFilter Lookup(Existing) " << REAL_INSERT_COUNT << " items." <<  std::endl;
    std::cout << "RdmaFilter True Positive Rate: " << (double)true_positive_count / REAL_INSERT_COUNT << std::endl;
    std::cout << "Lookup(Existing) Time(s): " << duration.count() / 1000.0 << std::endl;
    std::cout << "Throughput-Lookup(Existing)(op/s): " << REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    std::cout << "Payload-Bandwidth-Lookup(Existing)(MB/s): " << REAL_INSERT_COUNT * 1 * rdma_bf_cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;

    std::cout << std::endl << "RdmaFilter lookup non-existing keys..." << std::endl;
    start_time = std::chrono::high_resolution_clock::now();
    for (auto i : to_lookup) {
        if (!RdmaBF_Cli_lookup(&rdma_bf_cli, i)) {
            true_negative_count++;
        } else {
            false_positive_count++;
        }
    }
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "RdmaFilter Lookup(Non-Existing) " << LOOKUP_COUNT << " items." <<  std::endl;
    std::cout << "RdmaFilter True Negative Rate: " << (double)true_negative_count / LOOKUP_COUNT << std::endl;
    std::cout << "RdmaFilter False Positive Rate: " << (double)false_positive_count / LOOKUP_COUNT << std::endl;
    std::cout << "Lookup(False Positive) Time(s): " << duration.count() / 1000.0 << std::endl;
    std::cout << "Throughput-Lookup(False Positive)(op/s): " << LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;


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