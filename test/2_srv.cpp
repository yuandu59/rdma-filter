#include "rdma_bf.h"
#include "rdma_bbf.h"
#include "rdma_ohbbf.h"
#include "utils.h"
#include "rdma_cf.h"

#include <iostream>

#define RNIC_NAME "mlx5_0"
#define RNIC_PORT (1)
#define GID_INDEX (2)
#define TCP_PORT (18515)

#define CLIENT_COUNT (2)

#define INSERT_COUNT (1 << 26)

#define FALSE_POSITIVE_RATE ((double)1.0 / 512)
#define MUTEX_GRAN (1024)     // 每个锁占 8 字节，每个锁锁了 (粒度) 字节

#define BLOCK_SIZE (4)
#define MUTEX_GRAN_BLOCK (256)

#define BITS_PER_TAG_CF (16)
#define MUTEX_GRAN_BUCKET_CF (256)

int main(int argc, char **argv) {
    char cmd[16];
    int count_clients = CLIENT_COUNT;
#ifdef TOGGLE_LOCK_FREE
    count_clients = 1;
    std::cout << "[MODE] TOGGLE_LOCK_FREE" << std::endl;
#endif
    std::cout << "==== Experiment Begin ====" << std::endl;
    std::cout << "Current Time: " << get_current_time_string() << std::endl;

// ----------------------------------------------------------------------------------------------

    // struct RdmaBF_Srv srv;
    // RdmaBF_Srv_init(&srv, INSERT_COUNT, FALSE_POSITIVE_RATE, count_clients, RNIC_NAME, RNIC_PORT, TCP_PORT, GID_INDEX, MUTEX_GRAN);

    // for (int i = 0; i < count_clients; i++) {
    //     reliable_recv(srv.sockfd_list[i], cmd, 5);
    //     std::cout << "[Server] Received close message from client: " << i + 1 << "/" << count_clients << std::endl;
    // }

    // RdmaBF_Srv_destroy(&srv);

// ----------------------------------------------------------------------------------------------

            // struct RdmaBBF_Srv srv;
            // RdmaBBF_Srv_init(&srv, INSERT_COUNT, FALSE_POSITIVE_RATE, count_clients, RNIC_NAME, RNIC_PORT, TCP_PORT, GID_INDEX, MUTEX_GRAN_BLOCK, BLOCK_SIZE);

            // for (int i = 0; i < count_clients; i++) {
            //     reliable_recv(srv.sockfd_list[i], cmd, 5);
            //     std::cout << "[Server] Received close message from client: " << i + 1 << "/" << count_clients << std::endl;
            // }

            // RdmaBBF_Srv_destroy(&srv);

// ----------------------------------------------------------------------------------------------

    // struct RdmaOHBBF_Srv srv;
    // RdmaOHBBF_Srv_init(&srv, INSERT_COUNT, FALSE_POSITIVE_RATE, count_clients, RNIC_NAME, RNIC_PORT, TCP_PORT, GID_INDEX, MUTEX_GRAN_BLOCK, BLOCK_SIZE);

    // for (int i = 0; i < count_clients; i++) {
    //     reliable_recv(srv.sockfd_list[i], cmd, 5);
    //     std::cout << "[Server] Received close message from client: " << i + 1 << "/" << count_clients << std::endl;
    // }

    // RdmaOHBBF_Srv_destroy(&srv);

// ----------------------------------------------------------------------------------------------
    struct RdmaCF_Srv srv;
    RdmaCF_Srv_init(&srv, INSERT_COUNT, BITS_PER_TAG_CF, MUTEX_GRAN_BUCKET_CF, count_clients, RNIC_NAME, RNIC_PORT, TCP_PORT, GID_INDEX);
    
    sync_server(srv.list_sockfd);
    std::cout << "[Server] Initialization successfully!" << std::endl;

    sync_server(srv.list_sockfd);
    sync_server(srv.list_sockfd);
    sync_server(srv.list_sockfd);
    sync_server(srv.list_sockfd);
    for (int i = 0; i < count_clients; i++) {
        reliable_recv(srv.list_sockfd[i], cmd, 5);
        std::cout << "[Server] Received close message from client: " << i + 1 << "/" << count_clients << std::endl;
    }
    RdmaCF_Srv_destroy(&srv);

// ----------------------------------------------------------------------------------------------
    std::cout << "==== Experiment End ====" << std::endl;
    std::cout << "Current Time: " << get_current_time_string() << std::endl;
    return 0;
}
