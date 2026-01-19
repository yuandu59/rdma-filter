#ifndef __RDMA_CF_H__
#define __RDMA_CF_H__

#include <cstdint>
#include <stdlib.h>
#include <assert.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>
#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "rdma_common.h"
#include "hash.h"


struct rdma_conn_info_cf {
    uint32_t qp_num;
    uint32_t psn;               // initial packet sequence number
    union ibv_gid gid;
    uint64_t data_addr;         // server端位图地址
    uint32_t data_rkey;         // memory region key
    uint64_t mutex_addr;        // server端 mutex地址
    uint32_t mutex_rkey;        // mutex memory region key
    uint64_t victim_addr;       // server端 victim entry地址
    uint32_t victim_rkey;       // victim entry memory region key
    uint64_t mutex_victim_addr; // server端 victim mutex地址
    uint32_t mutex_victim_rkey; // victim mutex memory region key
} __attribute__((packed));


struct victim_entry {
    uint8_t tag;
    uint32_t index;
    bool used;
};

enum Status
{
    Ok = 0,
    NotFound = 1,
    NotEnoughSpace = 2,
    NotSupported = 3,
};

// ================= RDMA CF Client =================
struct RdmaCF_Cli {
	// Cuckoo Filter parameters
	unsigned int num_keys;
	unsigned int bits_per_tag;
	unsigned int k_max_kick;
    unsigned int mutex_gran_bucket;
    uint32_t count_buckets;
    uint32_t size_bucket;

	// RDMA resources
	ibv_context *ctx;
	ibv_pd *pd;
	ibv_cq *cq;
	ibv_qp *qp;

    uint64_t *buf_bucket;
    uint64_t *buf_mutex;
    victim_entry *buf_victim;
    uint64_t *buf_mutex_victim;

	std::vector<ibv_mr *> list_mr_bucket;
    std::vector<ibv_mr *> list_mr_mutex;
    ibv_mr *mr_victim;
    ibv_mr *mr_mutex_victim;
	std::vector<ibv_sge *> list_sge_bucket;
    std::vector<ibv_sge *> list_sge_mutex;
    ibv_sge *sge_victim;
    ibv_sge *sge_mutex_victim;

	// Remote info
	rdma_conn_info_cf remote_info;

	int sockfd;

    TwoIndependentMultiplyShift hasher;
};

void RdmaCF_Cli_init(struct RdmaCF_Cli *cli, unsigned int num_keys, unsigned int bits_per_tag, unsigned int kMaxCuckooCount, uint32_t mutex_gran_bucket, const char* server_ip, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index);
void RdmaCF_Cli_destroy(struct RdmaCF_Cli *cli);
Status RdmaCF_Cli_insert(struct RdmaCF_Cli *cli, uint64_t key);
Status RdmaCF_Cli_insert_impl(struct RdmaCF_Cli *cli, uint32_t bucket_index, uint16_t tag);
Status RdmaCF_Cli_lookup(struct RdmaCF_Cli *cli, uint64_t key);
Status RdmaCF_Cli_delete(struct RdmaCF_Cli *cli, uint64_t key);

int RdmaCF_Cli_conn(struct RdmaCF_Cli *cli, const char* server_ip, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index);
void RdmaCF_Cli_generate_index_tag_hash(struct RdmaCF_Cli *cli, uint64_t const key, uint32_t &index, uint16_t &tag);
uint32_t RdmaCF_Cli_get_alternate_index(struct RdmaCF_Cli *cli, uint32_t const &index, uint16_t const &tag);

bool RdmaCF_Cli_find_tag(struct RdmaCF_Cli *cli, uint16_t const &tag, int index_buf_bucket);
bool RdmaCF_Cli_delete_tag(struct RdmaCF_Cli *cli, uint16_t const &tag, int index_buf_bucket);
bool RdmaCF_Cli_insert_tag(struct RdmaCF_Cli *cli, uint16_t const &tag, int index_buf_bucket, bool kick_out, uint16_t &tag_pref);

int RdmaCF_Cli_read_victim(struct RdmaCF_Cli *cli);
int RdmaCF_Cli_write_victim(struct RdmaCF_Cli *cli);
int RdmaCF_Cli_lock_victim(struct RdmaCF_Cli *cli);
int RdmaCF_Cli_unlock_victim(struct RdmaCF_Cli *cli);

int RdmaCF_Cli_lock_bucket(struct RdmaCF_Cli *cli, uint32_t index_bucket, int index_buf_bucket);
int RdmaCF_Cli_unlock_bucket(struct RdmaCF_Cli *cli, uint32_t index_bucket, int index_buf_bucket);
int RdmaCF_Cli_read_bucket(struct RdmaCF_Cli *cli, uint32_t index_bucket, int index_buf_bucket);
int RdmaCF_Cli_write_bucket(struct RdmaCF_Cli *cli, uint32_t index_bucket, int index_buf_bucket);


// ================= RDMA CF Server =================
struct RdmaCF_Srv {
	unsigned int num_keys;
	unsigned int bits_per_tag;
    unsigned int mutex_gran_bucket;
	unsigned int count_clients_expected;

	// Cuckoo Filter data
	uint8_t *data;
    uint32_t size_data;
	uint64_t *mutex_list;
	uint32_t count_mutex;
    victim_entry *victim;
    uint64_t *mutex_victim;


	// RDMA resources
	ibv_context *ctx;
	ibv_pd *pd;
	ibv_cq *cq;
	std::vector<ibv_qp *> list_qp;
	ibv_mr *data_mr;
	ibv_mr *mutex_mr;
    ibv_mr *victim_mr;
    ibv_mr *mutex_victim_mr;

	std::vector<int> list_sockfd;
	std::vector<rdma_conn_info_cf> list_remote_info;

    bool use_hp;
};

void RdmaCF_Srv_init(struct RdmaCF_Srv *srv, unsigned int num_keys, unsigned int bits_per_tag, uint32_t mutex_gran_bucket, int client_count, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index);
void RdmaCF_Srv_destroy(struct RdmaCF_Srv *srv);

int RdmaCF_Srv_conn(struct RdmaCF_Srv *srv, int client_count, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index);

#endif // __RDMA_CF_H__