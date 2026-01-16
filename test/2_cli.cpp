#include "rdma_bf.h"
#include "rdma_bbf.h"
#include "rdma_ohbbf.h"
#include "utils.h"
#include "rdma_cf.h"

#include <iostream>
#include <unordered_set>
#include <chrono>


#define SERVER_IP "10.10.1.1"
#define RNIC_NAME "mlx5_0"
#define RNIC_PORT (1)
#define GID_INDEX (2)
#define TCP_PORT (18515)

#define INSERT_COUNT (1 << 26)
#define FALSE_POSITIVE_RATE ((double)1.0 / 512)
#define MUTEX_GRAN (1024)     // 每个锁占 8 字节，每个锁锁了 (粒度) 字节

#define BLOCK_SIZE (4)
#define MUTEX_GRAN_BLOCK (256)

#define LOOKUP_COUNT (1 << 16)
#define REAL_INSERT_COUNT (1 << 26)

#define BITS_PER_TAG_CF (16)
#define K_MAX_KICK_CF (10)
#define MUTEX_GRAN_BUCKET_CF (256)

int main(int argc, char **argv) {

    std::cout << "==== Experiment Begin ====" << std::endl;
    std::cout << "Current Time: " << get_current_time_string() << std::endl;
    int false_positive_count = 0, true_positive_count = 0, true_negative_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "= Dataset Preparing =" << std::endl;
    std::vector<uint64_t> to_insert = GenerateRandom64(REAL_INSERT_COUNT), to_lookup = {};
    std::unordered_set<uint64_t> to_insert_set(to_insert.begin(), to_insert.end());
    while (to_lookup.size() < LOOKUP_COUNT) {
        auto lookup_temp = GenerateRandom64(LOOKUP_COUNT - to_lookup.size());
        for (auto i : lookup_temp) {
            if (to_insert_set.find(i) == to_insert_set.end()) to_lookup.push_back(i);
        }
    }

    // std::cout << "=== RdmaBF Experiment ===" << std::endl;
    // struct RdmaBF_Cli cli;
    // RdmaBF_Cli_init(&cli, INSERT_COUNT, FALSE_POSITIVE_RATE, SERVER_IP, RNIC_NAME, RNIC_PORT, TCP_PORT, GID_INDEX, MUTEX_GRAN);

    // std::cout << "= Inserting =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_insert) {
    //     RdmaBF_Cli_insert(&cli, i);
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Inserted " << REAL_INSERT_COUNT << " items." << std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;

    // std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "Payload-Bandwidth(MB/s): " << 1.0 * REAL_INSERT_COUNT * 2 * cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;

    // std::cout << std::endl << "= Lookingup existing items =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_insert) {
    //     if (RdmaBF_Cli_lookup(&cli, i)) true_positive_count++;
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Lookup " << REAL_INSERT_COUNT << " existing items." <<  std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    // std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "Payload-Bandwidth(MB/s): " << 1.0 * REAL_INSERT_COUNT * 1 * cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;
    // std::cout << "True Positive Count: " << true_positive_count << std::endl;
    // std::cout << "True Positive Rate: " << (double)true_positive_count / REAL_INSERT_COUNT << std::endl;

    // std::cout << std::endl << "= Lookingup non-existing items =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_lookup) {
    //     if (RdmaBF_Cli_lookup(&cli, i)) false_positive_count++;
    //     else true_negative_count++;
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Lookup " << LOOKUP_COUNT << " non-existing items." <<  std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    // std::cout << "Throughput(op/s): " << 1.0 * LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "True Negative Count: " << true_negative_count << std::endl;
    // std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
    // std::cout << "False Positive Count: " << false_positive_count << std::endl;
    // std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;
    

    // reliable_send(cli.sockfd, "EXIT", 5);
    // RdmaBF_Cli_destroy(&cli);

// ----------------------------------------------------------------------------------------------

    // std::cout << "=== RdmaBBF Experiment ===" << std::endl;
    // struct RdmaBBF_Cli cli;
    // RdmaBBF_Cli_init(&cli, INSERT_COUNT, FALSE_POSITIVE_RATE, BLOCK_SIZE, SERVER_IP, RNIC_NAME, RNIC_PORT, TCP_PORT, GID_INDEX, MUTEX_GRAN_BLOCK);

    // std::cout << "= Inserting =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_insert) {
    //     RdmaBBF_Cli_insert(&cli, i);
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Inserted " << REAL_INSERT_COUNT << " items." << std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;

    // std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "Payload-Bandwidth(MB/s): " << 1.0 * REAL_INSERT_COUNT * 2 * cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;

    // std::cout << std::endl << "= Lookingup existing items =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_insert) {
    //     if (RdmaBBF_Cli_lookup(&cli, i)) true_positive_count++;
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Lookup " << REAL_INSERT_COUNT << " existing items." <<  std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    // std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "Payload-Bandwidth(MB/s): " << 1.0 * REAL_INSERT_COUNT * 1 * cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;
    // std::cout << "True Positive Count: " << true_positive_count << std::endl;
    // std::cout << "True Positive Rate: " << (double)true_positive_count / REAL_INSERT_COUNT << std::endl;

    // std::cout << std::endl << "= Lookingup non-existing items =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_lookup) {
    //     if (RdmaBBF_Cli_lookup(&cli, i)) false_positive_count++;
    //     else true_negative_count++;
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Lookup " << LOOKUP_COUNT << " non-existing items." <<  std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    // std::cout << "Throughput(op/s): " << 1.0 * LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "True Negative Count: " << true_negative_count << std::endl;
    // std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
    // std::cout << "False Positive Count: " << false_positive_count << std::endl;
    // std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;
    

    // reliable_send(cli.sockfd, "EXIT", 5);
    // RdmaBBF_Cli_destroy(&cli);

// ----------------------------------------------------------------------------------------------

    // std::cout << "=== RdmaOHBBF Experiment ===" << std::endl;
    // struct RdmaOHBBF_Cli cli;
    // RdmaOHBBF_Cli_init(&cli, INSERT_COUNT, FALSE_POSITIVE_RATE, BLOCK_SIZE, SERVER_IP, RNIC_NAME, RNIC_PORT, TCP_PORT, GID_INDEX, MUTEX_GRAN_BLOCK);

    // std::cout << "= Inserting =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_insert) {
    //     RdmaOHBBF_Cli_insert(&cli, i);
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Inserted " << REAL_INSERT_COUNT << " items." << std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;

    // std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "Payload-Bandwidth(MB/s): " << 1.0 * REAL_INSERT_COUNT * 2 * cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;

    // std::cout << std::endl << "= Lookingup existing items =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_insert) {
    //     if (RdmaOHBBF_Cli_lookup(&cli, i)) true_positive_count++;
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Lookup " << REAL_INSERT_COUNT << " existing items." <<  std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    // std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "Payload-Bandwidth(MB/s): " << 1.0 * REAL_INSERT_COUNT * 1 * cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;
    // std::cout << "True Positive Count: " << true_positive_count << std::endl;
    // std::cout << "True Positive Rate: " << (double)true_positive_count / REAL_INSERT_COUNT << std::endl;

    // std::cout << std::endl << "= Lookingup non-existing items =" << std::endl;
    // start_time = std::chrono::high_resolution_clock::now();
    // for (auto i : to_lookup) {
    //     if (RdmaOHBBF_Cli_lookup(&cli, i)) false_positive_count++;
    //     else true_negative_count++;
    // }
    // end_time = std::chrono::high_resolution_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Lookup " << LOOKUP_COUNT << " non-existing items." <<  std::endl;
    // std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    // std::cout << "Throughput(op/s): " << 1.0 * LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "True Negative Count: " << true_negative_count << std::endl;
    // std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
    // std::cout << "False Positive Count: " << false_positive_count << std::endl;
    // std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;

// ----------------------------------------------------------------------------------------------

    std::cout << "=== RdmaCF Experiment ===" << std::endl;
    struct RdmaCF_Cli cli;
    RdmaCF_Cli_init(&cli, INSERT_COUNT, BITS_PER_TAG_CF, K_MAX_KICK_CF, MUTEX_GRAN_BUCKET_CF, SERVER_IP, RNIC_NAME, RNIC_PORT, TCP_PORT, GID_INDEX);

    sync_client(cli.sockfd);
    std::cout << "[Client] Initialization successfully!" << std::endl;

    std::cout << "= Inserting =" << std::endl;
    sync_client(cli.sockfd);
    start_time = std::chrono::high_resolution_clock::now();
    for (auto i : to_insert) {
        if (Ok != RdmaCF_Cli_insert(&cli, i)) {
            std::cout << "Fail Insert(index): " << i << std::endl;
        }

        // debug
        // exit(0);
    }
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Inserted " << REAL_INSERT_COUNT << " items." << std::endl;
    std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;

    std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "Payload-Bandwidth(MB/s): " << 1.0 * REAL_INSERT_COUNT * 2 * cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;

    std::cout << std::endl << "= Lookingup existing items =" << std::endl;
    sync_client(cli.sockfd);
    start_time = std::chrono::high_resolution_clock::now();
    for (auto i : to_insert) {
        if (Ok == RdmaCF_Cli_lookup(&cli, i)) true_positive_count++;

        // debug
        // break;
    }
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Lookup " << REAL_INSERT_COUNT << " existing items." <<  std::endl;
    std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    // std::cout << "Payload-Bandwidth(MB/s): " << 1.0 * REAL_INSERT_COUNT * 1 * cli.k / duration.count() * 1000.0 / 1024 / 1024 << std::endl;
    std::cout << "True Positive Count: " << true_positive_count << std::endl;
    std::cout << "True Positive Rate: " << (double)true_positive_count / REAL_INSERT_COUNT << std::endl;

    std::cout << std::endl << "= Lookingup non-existing items =" << std::endl;
    sync_client(cli.sockfd);
    start_time = std::chrono::high_resolution_clock::now();
    for (auto i : to_lookup) {
        if (Ok == RdmaCF_Cli_lookup(&cli, i)) false_positive_count++;
        else true_negative_count++;

        // debug
        // exit(0);
    }
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Lookup " << LOOKUP_COUNT << " non-existing items." <<  std::endl;
    std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    std::cout << "Throughput(op/s): " << 1.0 * LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
    std::cout << "True Negative Count: " << true_negative_count << std::endl;
    std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
    std::cout << "False Positive Count: " << false_positive_count << std::endl;
    std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;

    std::cout << std::endl << "= Deleting items =" << std::endl;
    sync_client(cli.sockfd);
    start_time = std::chrono::high_resolution_clock::now();
    for (auto i : to_insert) {
        if (Ok != RdmaCF_Cli_delete(&cli, i)) {
            std::cout << "Fail Delete(index): " << i << std::endl;
        }
    }
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Lookup " << REAL_INSERT_COUNT << " non-existing items." <<  std::endl;
    std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    std::cout << "Throughput(op/s): " << 1.0 * REAL_INSERT_COUNT / duration.count() * 1000.0 << std::endl;
    

// ----------------------------------------------------------------------------------------------

    reliable_send(cli.sockfd, "EXIT", 5);
    RdmaCF_Cli_destroy(&cli);

    std::cout << "==== Experiment End ====" << std::endl;
    std::cout << "Current Time: " << get_current_time_string() << std::endl;
    return 0;
}
