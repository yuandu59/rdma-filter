#include "dram_ohbbf.h"

#include <iostream>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include "murmur3.h"

using namespace std;

#define bit_set(v, n) ((v)[(n) >> 3] |= (0x1 << (0x7 - ((n)&0x7))))
#define bit_get(v, n) ((v)[(n) >> 3] & (0x1 << (0x7 - ((n)&0x7))))
#define bit_clr(v, n) ((v)[(n) >> 3] &= ~(0x1 << (0x7 - ((n)&0x7))))

#define SEED 2568

void DramOHBBF_init(struct DramOHBBF *dram_ohbbf, unsigned int n, double fpr, unsigned int block_size)
{
    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;

    memset(dram_ohbbf, 0, sizeof(*dram_ohbbf));
    dram_ohbbf->m = ((int(m) >> 3) + 1) << 3;
    dram_ohbbf->k = ceil(k);
    dram_ohbbf->bit_vector = (uint8_t *)calloc(dram_ohbbf->m >> 3, sizeof(uint8_t));

    dram_ohbbf->block_size = block_size;
    dram_ohbbf->block_count = dram_ohbbf->m / (block_size << 3);
    return;
}

void DramOHBBF_destroy(struct DramOHBBF *dram_ohbbf)
{
    free(dram_ohbbf->bit_vector);
}

int DramOHBBF_insert(struct DramOHBBF *dram_ohbbf, uint64_t key_)
{
    void *key = &key_;

    uint32_t hash, hash_;
    murmur3_hash32(key, 8, SEED, &hash);
    uint32_t block_index = hash % dram_ohbbf->block_count;
    uint32_t block_offset = block_index * dram_ohbbf->block_size;

    for (uint32_t i = 0; i < dram_ohbbf->k; i++)
    {
        hash_ = (hash >> 16) ^ (hash << i);
        bit_set(dram_ohbbf->bit_vector + block_offset, hash_ % (dram_ohbbf->block_size << 3));
    }
    return 1;
}

int DramOHBBF_lookup(struct DramOHBBF *dram_ohbbf, uint64_t key_)
{
    void *key = &key_;

    uint32_t hash, hash_;
    murmur3_hash32(key, 8, SEED, &hash);
    uint32_t block_index = hash % dram_ohbbf->block_count;
    uint32_t block_offset = block_index * dram_ohbbf->block_size;

    for (uint32_t i = 0; i < dram_ohbbf->k; i++)
    {
        hash_ = (hash >> 16) ^ (hash << i);
        if (!bit_get(dram_ohbbf->bit_vector + block_offset, hash_ % (dram_ohbbf->block_size << 3)))
            return 0;
    }
    return 1;
}

void DramOHBBF_clear(struct DramOHBBF *dram_ohbbf)
{
    memset(dram_ohbbf->bit_vector, 0, dram_ohbbf->m >> 3);
}