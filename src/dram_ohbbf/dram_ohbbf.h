#ifndef __DRAM_OHBBF_H__
#define __DRAM_OHBBF_H__

#include <stdint.h>
#include <stdlib.h>

struct DramOHBBF
{
    unsigned int m;
    unsigned int k;
    uint8_t *bit_vector;
    unsigned int block_size;
    unsigned int block_count;
};

void DramOHBBF_init(struct DramOHBBF *dram_ohbbf, unsigned int n, double fpr, unsigned int block_size);

void DramOHBBF_destroy(struct DramOHBBF *dram_ohbbf);

int DramOHBBF_insert(struct DramOHBBF *dram_ohbbf, uint64_t key);

int DramOHBBF_lookup(struct DramOHBBF *dram_ohbbf, uint64_t key);

void DramOHBBF_clear(struct DramOHBBF *dram_ohbbf);

#endif /* __DRAM_OHBBF_H__ */