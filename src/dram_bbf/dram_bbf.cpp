#include "dram_bbf.h"

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

void DramBBF_init(struct DramBBF *dram_bbf, unsigned int n, double fpr, unsigned int block_size)
{
    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;

    memset(dram_bbf, 0, sizeof(*dram_bbf));
    dram_bbf->m = ((int(m) >> 3) + 1) << 3;
    dram_bbf->k = ceil(k);
    dram_bbf->bit_vector = (uint8_t *)calloc(dram_bbf->m >> 3, sizeof(uint8_t));

    dram_bbf->block_size = block_size;
    dram_bbf->block_count = dram_bbf->m / (block_size << 3);

    return;
}

void DramBBF_destroy(struct DramBBF *dram_bbf)
{
    free(dram_bbf->bit_vector);
}

int DramBBF_insert(struct DramBBF *dram_bbf, uint64_t key_)
{
    void *key = &key_;

    uint32_t hash, hash_;
    murmur3_hash32(key, 8, SEED, &hash);
    uint32_t block_index = hash % dram_bbf->block_count;
    uint32_t block_offset = block_index * dram_bbf->block_size;

    for (uint32_t i = 0; i < dram_bbf->k; i++)
    {
        murmur3_hash32(key, 8, i, &hash_);
        bit_set(dram_bbf->bit_vector + block_offset, hash_ % (dram_bbf->block_size << 3));
    }
    return 1;
}

int DramBBF_lookup(struct DramBBF *dram_bbf, uint64_t key_)
{
    void *key = &key_;

    uint32_t hash, hash_;
    murmur3_hash32(key, 8, SEED, &hash);
    uint32_t block_index = hash % dram_bbf->block_count;
    uint32_t block_offset = block_index * dram_bbf->block_size;

    for (uint32_t i = 0; i < dram_bbf->k; i++)
    {
        murmur3_hash32(key, 8, i, &hash_);
        if (!bit_get(dram_bbf->bit_vector + block_offset, hash_ % (dram_bbf->block_size << 3)))
            return 0;
    }
    return 1;
}

void DramBBF_clear(struct DramBBF *dram_bbf)
{
    memset(dram_bbf->bit_vector, 0, dram_bbf->m >> 3);
}