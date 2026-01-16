#include "rdma_cf.h"
#include "utils.h"

#define mask4(x) ((x) & ((1ULL << (4)) - 1))
#define mask8(x) ((x) & ((1ULL << (8)) - 1))
#define mask16(x) ((x) & ((1ULL << (16)) - 1))
#define mask32(x) ((x) & ((1ULL << (32)) - 1))
#define mask64(x) ((x) & (0xFFFFFFFFFFFFFFFFULL))

#define haszero4(x) (((x)-0x1111ULL) & (~(x)) & 0x8888ULL)
#define hasvalue4(x, n) (haszero4((x) ^ (0x1111ULL * (n))))
#define haszero8(x) (((x)-0x01010101ULL) & (~(x)) & 0x80808080ULL)
#define hasvalue8(x, n) (haszero8((x) ^ (0x01010101ULL * (n))))
#define haszero16(x) (((x)-0x0001000100010001ULL) & (~(x)) & 0x8000800080008000ULL)
#define hasvalue16(x, n) (haszero16((x) ^ (0x0001000100010001ULL * (n))))

#define TAGS_PER_BUCKET (4)
#define COUNT_BUF_BUCKET (3)


void RdmaCF_Cli_init(struct RdmaCF_Cli *cli, unsigned int num_keys, unsigned int bits_per_tag, unsigned int kMaxCuckooCount, uint32_t mutex_gran_bucket, const char* server_ip, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index) {
    assert_else(bits_per_tag == 4 || bits_per_tag == 8 || bits_per_tag == 16, "bits_per_tag must be 4, 8, or 16");
    memset(cli, 0, sizeof(*cli));
    cli->num_keys = num_keys;
    cli->bits_per_tag = bits_per_tag;
    cli->k_max_kick = kMaxCuckooCount;
    cli->mutex_gran_bucket = mutex_gran_bucket;
    cli->hasher = TwoIndependentMultiplyShift();

    cli->count_buckets = upperpower2(std::max<uint64_t>(1, num_keys / TAGS_PER_BUCKET));
    double frac = (double)num_keys / cli->count_buckets / TAGS_PER_BUCKET;
    if (frac > 0.96) cli->count_buckets <<= 1;
    cli->size_bucket = (bits_per_tag * TAGS_PER_BUCKET) >> 3;

    // allocate
    alloc_aligned_64((void**)&cli->buf_bucket, COUNT_BUF_BUCKET * sizeof(uint64_t));
    alloc_aligned_64((void**)&cli->buf_mutex, COUNT_BUF_BUCKET * sizeof(uint64_t));
    alloc_aligned_64((void**)&cli->buf_victim, sizeof(victim_entry));
    alloc_aligned_64((void**)&cli->buf_mutex_victim, sizeof(uint64_t));

    // connect
    RdmaCF_Cli_conn(cli, server_ip, name_dev, rnic_port, tcp_port, gid_index);
    return;
}

int RdmaCF_Cli_conn(struct RdmaCF_Cli *cli, const char* server_ip, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index) {
    // open ctx and create pd and cq
    cli->ctx = open_rdma_ctx(name_dev);
    cli->pd = ibv_alloc_pd(cli->ctx);
    cli->cq = ibv_create_cq(cli->ctx, 16, NULL, NULL, 0);

    // register mr
    cli->list_mr_bucket = std::vector<ibv_mr *>(COUNT_BUF_BUCKET);
    cli->list_mr_mutex = std::vector<ibv_mr *>(COUNT_BUF_BUCKET);
    for (int i = 0; i < COUNT_BUF_BUCKET; ++i) {
        cli->list_mr_bucket[i] = ibv_reg_mr(cli->pd, &(cli->buf_bucket[i]), cli->size_bucket, MR_FLAGS_RW);
        cli->list_mr_mutex[i] = ibv_reg_mr(cli->pd, &(cli->buf_mutex[i]), sizeof(uint64_t), MR_FLAGS_ATOMIC);
    }
    cli->mr_victim = ibv_reg_mr(cli->pd, cli->buf_victim, sizeof(victim_entry), MR_FLAGS_RW);
    cli->mr_mutex_victim = ibv_reg_mr(cli->pd, cli->buf_mutex_victim, sizeof(uint64_t), MR_FLAGS_ATOMIC);

    // create sge
    cli->list_sge_bucket = std::vector<ibv_sge *>(COUNT_BUF_BUCKET);
    cli->list_sge_mutex = std::vector<ibv_sge *>(COUNT_BUF_BUCKET);
    for (int i = 0; i < COUNT_BUF_BUCKET; ++i) {
        cli->list_sge_bucket[i] = create_sge(cli->list_mr_bucket[i]);
        cli->list_sge_mutex[i] = create_sge(cli->list_mr_mutex[i]);
    }
    cli->sge_victim = create_sge(cli->mr_victim);
    cli->sge_mutex_victim = create_sge(cli->mr_mutex_victim);

    // create qp and modify to init
    cli->qp = create_rc_qp(cli->pd, cli->cq);
    modify_init_qp(cli->qp, rnic_port);

    // debug
    // assert_else(false, "[DEBUG] QP num: " + std::to_string(cli->qp->qp_num));

    // create local_info
    rdma_conn_info_cf *local_info = create_rdma_conn_info<rdma_conn_info_cf>(cli->ctx, rnic_port, gid_index);
    local_info->qp_num = cli->qp->qp_num;

    // exchange info over TCP
    cli->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    cli->remote_info = {};
    tcp_exchange_client<rdma_conn_info_cf>(tcp_port, server_ip, cli->sockfd, local_info, &(cli->remote_info));

    // modify qp to rtr and rts
    modify_rtr_qp(cli->qp, cli->remote_info.qp_num, cli->remote_info.psn, cli->remote_info.gid, gid_index, rnic_port);
    modify_rts_qp(cli->qp, local_info->psn);

    return 1;
}

void RdmaCF_Cli_destroy(struct RdmaCF_Cli *cli) {
    if (cli->sockfd)    close(cli->sockfd);
    if (cli->qp)        ibv_destroy_qp(cli->qp);
    for (int i = 0; i < COUNT_BUF_BUCKET; ++i) {
        free(cli->list_sge_bucket[i]);
        free(cli->list_sge_mutex[i]);
    }
    if (cli->sge_victim)        free(cli->sge_victim);
    if (cli->sge_mutex_victim)  free(cli->sge_mutex_victim);
    for (int i = 0; i < COUNT_BUF_BUCKET; ++i) {
        ibv_dereg_mr(cli->list_mr_bucket[i]);
        ibv_dereg_mr(cli->list_mr_mutex[i]);
    }
    if (cli->mr_victim)         ibv_dereg_mr(cli->mr_victim);
    if (cli->mr_mutex_victim)   ibv_dereg_mr(cli->mr_mutex_victim);
    if (cli->buf_bucket)        free(cli->buf_bucket);
    if (cli->buf_mutex)         free(cli->buf_mutex);
    if (cli->buf_victim)        free(cli->buf_victim);
    if (cli->buf_mutex_victim)  free(cli->buf_mutex_victim);
    if (cli->cq)    ibv_destroy_cq(cli->cq);
    if (cli->pd)    ibv_dealloc_pd(cli->pd);
    if (cli->ctx)   ibv_close_device(cli->ctx);
    return;
}

Status RdmaCF_Cli_insert(struct RdmaCF_Cli *cli, uint64_t key) {
    RdmaCF_Cli_lock_victim(cli);
    RdmaCF_Cli_read_victim(cli);
    if (cli->buf_victim->used) {
        RdmaCF_Cli_unlock_victim(cli);
        return NotEnoughSpace;
    }

    uint32_t index;
    uint16_t tag;
    RdmaCF_Cli_generate_index_tag_hash(cli, key, index, tag);
    auto res = RdmaCF_Cli_insert_impl(cli, index, tag);
    RdmaCF_Cli_unlock_victim(cli);
    return res;
}

// when victim has been locked
Status RdmaCF_Cli_insert_impl(struct RdmaCF_Cli *cli, uint32_t bucket_index, uint16_t tag) {
    uint32_t index_curr = bucket_index;
    uint16_t tag_curr = tag, tag_prev;
    bool inserted = false, kick_out;

    for (int count = 0; count < cli->k_max_kick; count++) {
        kick_out = count > 0;
        tag_prev = 0;
        RdmaCF_Cli_lock_bucket(cli, index_curr, 2);
        RdmaCF_Cli_read_bucket(cli, index_curr, 2);
        inserted = RdmaCF_Cli_insert_tag(cli, tag_curr, 2, kick_out, tag_prev);
        RdmaCF_Cli_write_bucket(cli, index_curr, 2);
        RdmaCF_Cli_unlock_bucket(cli, index_curr, 2);
        if (inserted) return Ok;
        if (kick_out) {
            tag_curr = tag_prev;
        }
        index_curr = RdmaCF_Cli_get_alternate_index(cli, index_curr, tag_prev);
    }
    cli->buf_victim->used = true;
    cli->buf_victim->index = index_curr;
    cli->buf_victim->tag = tag_curr;
    RdmaCF_Cli_write_victim(cli);
    return Ok;
}

Status RdmaCF_Cli_lookup(struct RdmaCF_Cli *cli, uint64_t key) {
    bool found = false;
    uint32_t index1, index2;
    uint16_t tag;
    RdmaCF_Cli_generate_index_tag_hash(cli, key, index1, tag);
    index2 = RdmaCF_Cli_get_alternate_index(cli, index1, tag);

    // debug
    // assert_else(false, "[DEBUG] Lookup index1: " + std::to_string(index1) + ", index2: " + std::to_string(index2) + ", tag: " + std::to_string(tag));

    RdmaCF_Cli_lock_victim(cli);
    RdmaCF_Cli_read_victim(cli);
    found = cli->buf_victim->used && cli->buf_victim->tag == tag &&
            (cli->buf_victim->index == index1 || cli->buf_victim->index == index2);
    RdmaCF_Cli_unlock_victim(cli);

    // debug
    // assert_else(found == false, "[DEBUG] found in victim.");

    if (found) return Ok;

    // debug
    // assert_else(false, "[DEBUG] Not found in victim.");

    RdmaCF_Cli_lock_bucket(cli, index1, 0);
    RdmaCF_Cli_read_bucket(cli, index1, 0);
    found = RdmaCF_Cli_find_tag(cli, tag, 0);
    if (found) {
        RdmaCF_Cli_unlock_bucket(cli, index1, 0);

        // debug
        // assert_else(false, "[DEBUG] found in index1.");

        return Ok;
    }

    // debug
    // assert_else(false, "[DEBUG] Not found in index1.");

    RdmaCF_Cli_lock_bucket(cli, index2, 1);
    RdmaCF_Cli_read_bucket(cli, index2, 1);
    found = RdmaCF_Cli_find_tag(cli, tag, 1);
    RdmaCF_Cli_unlock_bucket(cli, index1, 0);
    RdmaCF_Cli_unlock_bucket(cli, index2, 1);

    // debug
    // assert_else(found == false, "[DEBUG] found in index2.");
    // assert_else(found == true, "[DEBUG] not found in index2.");

    if (found) return Ok;
    else return NotFound;
}

Status RdmaCF_Cli_delete(struct RdmaCF_Cli *cli, uint64_t key) {
    uint32_t index1, index2;
    uint16_t tag;
    bool found = false, access2 = false;
    RdmaCF_Cli_generate_index_tag_hash(cli, key, index1, tag);
    index2 = RdmaCF_Cli_get_alternate_index(cli, index1, tag);

    RdmaCF_Cli_lock_bucket(cli, index1, 0);
    RdmaCF_Cli_read_bucket(cli, index1, 0);
    if (RdmaCF_Cli_delete_tag(cli, tag, 0)) {
        found = true;
        RdmaCF_Cli_write_bucket(cli, index1, 0);
    }
    if (false == found) {
        access2 = true;
        RdmaCF_Cli_lock_bucket(cli, index2, 1);
        RdmaCF_Cli_read_bucket(cli, index2, 1);
        if (RdmaCF_Cli_delete_tag(cli, tag, 1)) {
            found = true;
            RdmaCF_Cli_write_bucket(cli, index2, 1);
        }
    }
    RdmaCF_Cli_lock_victim(cli);
    RdmaCF_Cli_read_victim(cli);
    if (found) {
        if (cli->buf_victim->used) {
            cli->buf_victim->used = false;
            RdmaCF_Cli_write_victim(cli);
            uint64_t index = cli->buf_victim->index;
            uint16_t tag = cli->buf_victim->tag;
            RdmaCF_Cli_insert_impl(cli, index, tag);
        }
        RdmaCF_Cli_unlock_victim(cli);
        RdmaCF_Cli_unlock_bucket(cli, index1, 0);
        if (access2) RdmaCF_Cli_unlock_bucket(cli, index2, 1);
        return Ok;
    }
    
    if (cli->buf_victim->used && cli->buf_victim->tag == tag &&
        (cli->buf_victim->index == index1 || cli->buf_victim->index == index2)) {
        cli->buf_victim->used = false;
        RdmaCF_Cli_write_victim(cli);
        found = true;
    }
    RdmaCF_Cli_unlock_victim(cli);
    RdmaCF_Cli_unlock_bucket(cli, index1, 0);
    if (access2) RdmaCF_Cli_unlock_bucket(cli, index2, 1);
    if (found) return Ok;
    return NotFound;
}

void RdmaCF_Cli_generate_index_tag_hash(struct RdmaCF_Cli *cli, uint64_t const key, uint32_t &index, uint16_t &tag) {
    const uint64_t hash = cli->hasher(key);
    index = (hash >> 32) & (cli->count_buckets - 1);
    tag = hash & ((1ULL << cli->bits_per_tag) - 1);
    tag += (tag == 0);
    return;
}

uint32_t RdmaCF_Cli_get_alternate_index(struct RdmaCF_Cli *cli, uint32_t const &index, uint16_t const &tag) {
    return (index ^ (tag * 0x5bd1e995)) & (cli->count_buckets - 1);
}

bool RdmaCF_Cli_find_tag(struct RdmaCF_Cli *cli, uint16_t const &tag, int index_buf_bucket) {
    uint64_t data_bucket = cli->buf_bucket[index_buf_bucket];
    if (cli->bits_per_tag == 4) {
        return hasvalue4(mask16(data_bucket), mask4(tag));
    }
    else if (cli->bits_per_tag == 8) {
        return hasvalue8(mask32(data_bucket), mask8(tag));
    }
    else if (cli->bits_per_tag == 16) {
        return hasvalue16(mask64(data_bucket), mask16(tag));
    }
    assert_else(false, "bits_per_tag must be 4, 8, or 16");
    return false;
}

bool RdmaCF_Cli_delete_tag(struct RdmaCF_Cli *cli, uint16_t const &tag, int index_buf_bucket) {
    uint64_t data_bucket = cli->buf_bucket[index_buf_bucket];
    uint64_t tag64 = ((1ULL << cli->bits_per_tag) - 1) & tag;
    for (int i = 0; i < TAGS_PER_BUCKET; i++) {
        uint64_t slot_mask = ((1ULL << cli->bits_per_tag) - 1) << (i * cli->bits_per_tag);
        if (tag64 == ((data_bucket & slot_mask) >> (i * cli->bits_per_tag))) {
            cli->buf_bucket[index_buf_bucket] &= ~slot_mask;
            return true;
        }
    }
    return false;
}

bool RdmaCF_Cli_insert_tag(struct RdmaCF_Cli *cli, uint16_t const &tag, int index_buf_bucket, bool kick_out, uint16_t &tag_pref) {
    uint64_t data_bucket = cli->buf_bucket[index_buf_bucket];
    uint64_t tag64 = ((1ULL << cli->bits_per_tag) - 1) & tag;
    for (int i = 0; i < TAGS_PER_BUCKET; i++) {
        uint64_t slot_mask = ((1ULL << cli->bits_per_tag) - 1) << (i * cli->bits_per_tag);
        if (((data_bucket & slot_mask) >> (i * cli->bits_per_tag)) == 0) {
            cli->buf_bucket[index_buf_bucket] |= (tag64 << (i * cli->bits_per_tag));
            return true;
        }
    }
    if (kick_out) {
        int i = rand() % TAGS_PER_BUCKET;
        uint64_t slot_mask = ((1ULL << cli->bits_per_tag) - 1) << (i * cli->bits_per_tag);
        tag_pref = (uint16_t)((data_bucket & slot_mask) >> (i * cli->bits_per_tag));
        cli->buf_bucket[index_buf_bucket] &= ~slot_mask;
        cli->buf_bucket[index_buf_bucket] |= (tag64 << (i * cli->bits_per_tag));
    }
    return false;
}

// when victim has been locked
int RdmaCF_Cli_read_victim(struct RdmaCF_Cli *cli) {
    rdma_one_side(cli->qp, 1, cli->sge_victim, cli->remote_info.victim_addr, cli->remote_info.victim_rkey, IBV_WR_RDMA_READ);
    check_cq(cli->cq, 1);
    return 1;
}

// when victim has been locked
int RdmaCF_Cli_write_victim(struct RdmaCF_Cli *cli) {
    rdma_one_side(cli->qp, 2, cli->sge_victim, cli->remote_info.victim_addr, cli->remote_info.victim_rkey, IBV_WR_RDMA_WRITE);
    check_cq(cli->cq, 2);
    return 1;
}

int RdmaCF_Cli_lock_victim(struct RdmaCF_Cli *cli) {
#ifndef TOGGLE_LOCK_FREE
    auto res_cas = rdma_atomic_cas(cli->qp, 200, cli->sge_mutex_victim, cli->cq, cli->remote_info.mutex_victim_addr, cli->remote_info.mutex_victim_rkey, 0, 1);
    assert_else(res_cas == 1, "Failed to lock victim mutex");
#endif
    return 1;
}

// not atomic operation
int RdmaCF_Cli_unlock_victim(struct RdmaCF_Cli *cli) {
#ifndef TOGGLE_LOCK_FREE
    *(cli->buf_mutex_victim) = (uint64_t)0;
    rdma_one_side(cli->qp, 201, cli->sge_mutex_victim, cli->remote_info.mutex_victim_addr, cli->remote_info.mutex_victim_rkey, IBV_WR_RDMA_WRITE);
    check_cq(cli->cq, 201);
#endif
    return 1;
}

int RdmaCF_Cli_lock_bucket(struct RdmaCF_Cli *cli, uint32_t index_bucket, int index_buf_bucket) {
#ifndef TOGGLE_LOCK_FREE
    uint64_t addr_mutex = cli->remote_info.mutex_addr + (index_bucket / cli->mutex_gran_bucket) * sizeof(uint64_t);
    auto res_cas = rdma_atomic_cas(cli->qp, 100, cli->list_sge_mutex[index_buf_bucket], cli->cq, addr_mutex, cli->remote_info.mutex_rkey, 0, 1);
    assert_else(res_cas == 1, "Failed to lock bucket mutex");

    // debug
    assert_else(res_cas == 1, "Failed to lock bucket mutex index: " + std::to_string(index_bucket));
#endif
    return 1;
}

// not atomic operation
int RdmaCF_Cli_unlock_bucket(struct RdmaCF_Cli *cli, uint32_t index_bucket, int index_buf_bucket) {
#ifndef TOGGLE_LOCK_FREE
    uint64_t addr_mutex = cli->remote_info.mutex_addr + (index_bucket / cli->mutex_gran_bucket) * sizeof(uint64_t);
    cli->buf_mutex[index_buf_bucket] = (uint64_t)0;
    rdma_one_side(cli->qp, 101, cli->list_sge_mutex[index_buf_bucket], addr_mutex, cli->remote_info.mutex_rkey, IBV_WR_RDMA_WRITE);
    check_cq(cli->cq, 101);
#endif
    return 1;
}

// when the bucket has been locked
int RdmaCF_Cli_read_bucket(struct RdmaCF_Cli *cli, uint32_t index_bucket, int index_buf_bucket) {
    uint64_t addr_bucket = cli->remote_info.data_addr + index_bucket * cli->size_bucket;

    rdma_one_side(cli->qp, 1, cli->list_sge_bucket[index_buf_bucket], addr_bucket, cli->remote_info.data_rkey, IBV_WR_RDMA_READ);
    check_cq(cli->cq, 1);
    return 1;
}

// when the bucket has been locked
int RdmaCF_Cli_write_bucket(struct RdmaCF_Cli *cli, uint32_t index_bucket, int index_buf_bucket) {
    uint64_t addr_bucket = cli->remote_info.data_addr + index_bucket * cli->size_bucket;

    rdma_one_side(cli->qp, 2, cli->list_sge_bucket[index_buf_bucket], addr_bucket, cli->remote_info.data_rkey, IBV_WR_RDMA_WRITE);
    check_cq(cli->cq, 2);
    return 1;
}


// ================= RDMA CF Server =================
void RdmaCF_Srv_init(struct RdmaCF_Srv *srv, unsigned int num_keys, unsigned int bits_per_tag, uint32_t mutex_gran_bucket, int client_count, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index) {
    assert_else(bits_per_tag == 4 || bits_per_tag == 8 || bits_per_tag == 16, "bits_per_tag must be 4, 8, or 16");
    memset(srv, 0, sizeof(*srv));

    srv->num_keys = num_keys;
    srv->mutex_gran_bucket = mutex_gran_bucket;
    srv->count_clients_expected = client_count;
    srv->list_sockfd = std::vector<int>(client_count);
    srv->list_remote_info = std::vector<rdma_conn_info_cf>(client_count);
    
    // size_data
    size_t count_buckets = upperpower2(std::max<uint64_t>(1, num_keys / TAGS_PER_BUCKET));
    double frac = (double)num_keys / count_buckets / TAGS_PER_BUCKET;
    if (frac > 0.96) count_buckets <<= 1;
    uint32_t size_bucket = (bits_per_tag * TAGS_PER_BUCKET) >> 3;
    srv->size_data = count_buckets * size_bucket;

    // count_mutex
#ifndef TOGGLE_LOCK_FREE // enable mutex
    srv->count_mutex = (count_buckets + mutex_gran_bucket - 1) / mutex_gran_bucket;
#else // disable mutex
    srv->count_mutex = 1;
#endif

    // allocate space
    alloc_aligned_64((void**)&srv->data, srv->size_data);
    alloc_aligned_64((void**)&srv->mutex_list, srv->count_mutex * sizeof(uint64_t));
    alloc_aligned_64((void**)&srv->victim, sizeof(victim_entry));
    alloc_aligned_64((void**)&srv->mutex_victim, sizeof(uint64_t));

    // set initial value
    srv->victim->used = false;
    
    // out print
    std::cout << "[Server] Cuckoo Filter size(MB): " << srv->size_data / 1024.0 / 1024.0 << std::endl;
    std::cout << "[Server] Mutex list size(KB): " << srv->count_mutex * sizeof(uint64_t) / 1024.0 << std::endl;

    // connect
    RdmaCF_Srv_conn(srv, client_count, name_dev, rnic_port, tcp_port, gid_index);

    // debug
    for (int i = 0; i < client_count; ++i) {
        std::cout << "[Server] Client " << i << " QP num: " << srv->list_remote_info[i].qp_num << std::endl;
        std::cout << "[Server] " << i << " client pairred qp num: " << srv->list_qp[i]->qp_num << std::endl;
    }
    return;
}

int RdmaCF_Srv_conn(struct RdmaCF_Srv *srv, int client_count, const char* name_dev, uint8_t rnic_port, uint32_t tcp_port, uint8_t gid_index) {
    // open ctx, create pd, cq
    srv->ctx = open_rdma_ctx(name_dev);
    srv->pd = ibv_alloc_pd(srv->ctx);
    srv->cq = ibv_create_cq(srv->ctx, 16, NULL, NULL, 0);

    // register mr
    srv->data_mr = ibv_reg_mr(srv->pd, srv->data, srv->size_data, MR_FLAGS_RW);
    srv->mutex_mr = ibv_reg_mr(srv->pd, srv->mutex_list, srv->count_mutex * sizeof(uint64_t), MR_FLAGS_ATOMIC);
    srv->victim_mr = ibv_reg_mr(srv->pd, srv->victim, sizeof(victim_entry), MR_FLAGS_RW);
    srv->mutex_victim_mr = ibv_reg_mr(srv->pd, srv->mutex_victim, sizeof(uint64_t), MR_FLAGS_ATOMIC);

    // init list_qp
    srv->list_qp = std::vector<ibv_qp *>(client_count);
    for (int i = 0; i < client_count; ++i) {
        srv->list_qp[i] = create_rc_qp(srv->pd, srv->cq);
        modify_init_qp(srv->list_qp[i], rnic_port);
    }

    // create and fill local_info
    std::vector<rdma_conn_info_cf *> list_local_info(client_count);
    for (int i = 0; i < client_count; i++) {
        list_local_info[i] = create_rdma_conn_info<rdma_conn_info_cf>(srv->ctx, rnic_port, gid_index);
        list_local_info[i]->data_addr = (uintptr_t)srv->data;
        list_local_info[i]->data_rkey = srv->data_mr->rkey;
        list_local_info[i]->mutex_addr = (uintptr_t)srv->mutex_list;
        list_local_info[i]->mutex_rkey = srv->mutex_mr->rkey;
        list_local_info[i]->victim_addr = (uintptr_t)srv->victim;
        list_local_info[i]->victim_rkey = srv->victim_mr->rkey;
        list_local_info[i]->mutex_victim_addr = (uintptr_t)srv->mutex_victim;
        list_local_info[i]->mutex_victim_rkey = srv->mutex_victim_mr->rkey;
        list_local_info[i]->qp_num = srv->list_qp[i]->qp_num;
    }

    // exchange info over TCP
    tcp_exchange_server<rdma_conn_info_cf>(tcp_port, client_count, srv->list_sockfd, srv->list_remote_info, list_local_info);

    // modify QP to RTR and RTS
    for (int i = 0; i < client_count; i++) {
        modify_rtr_qp(srv->list_qp[i], srv->list_remote_info[i].qp_num, srv->list_remote_info[i].psn, srv->list_remote_info[i].gid, gid_index, rnic_port);
        modify_rts_qp(srv->list_qp[i], srv->list_remote_info[i].psn);
    }
    return 1;
}

void RdmaCF_Srv_destroy(struct RdmaCF_Srv *srv) {
    for (int i = 0; i < srv->count_clients_expected; i++) {
        if(srv->list_qp[i]) ibv_destroy_qp(srv->list_qp[i]);
        if(srv->list_sockfd[i] >= 0) close(srv->list_sockfd[i]);
    }
    if(srv->data_mr)            ibv_dereg_mr(srv->data_mr);
    if(srv->mutex_mr)           ibv_dereg_mr(srv->mutex_mr);
    if(srv->victim_mr)          ibv_dereg_mr(srv->victim_mr);
    if(srv->mutex_victim_mr)    ibv_dereg_mr(srv->mutex_victim_mr);
    if(srv->data)           free(srv->data);
    if(srv->mutex_list)     free(srv->mutex_list);
    if(srv->victim)         free(srv->victim);
    if(srv->mutex_victim)   free(srv->mutex_victim);
    if(srv->cq)     ibv_destroy_cq(srv->cq);
    if(srv->pd)     ibv_dealloc_pd(srv->pd);
    if(srv->ctx)    ibv_close_device(srv->ctx);
}
